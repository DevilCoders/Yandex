import pandas as pd, datetime, ast, os, sys, pymysql, logging, requests, time
from requests.exceptions import HTTPError

module_path = os.path.abspath(os.path.join('/home/ktereshin/yandex/arcadia/cloud/analytics/python/work'))
if module_path not in sys.path:
    sys.path.append(module_path)
module_path = os.path.abspath(os.path.join('/home/ktereshin/yandex/cron'))
if module_path not in sys.path:
    sys.path.append(module_path)
from data_loader import clickhouse
from global_variables import (
    metrika_clickhouse_param_dict,
    cloud_clickhouse_param_dict
)
from nile.api.v1 import (
    clusters,
    aggregators as na,
    extractors as ne,
    filters as nf,
    Record
)
from creds import (
    yt_creds,
    metrika_creds,
    yc_ch_creds,
    crm_sql_creds,
    stat_creds
)


def execute_query(query, cluster, alias, token, timeout=1200):
    proxy = "http://{}.yt.yandex.net".format(cluster)
    s = requests.Session()
    url = "{proxy}/query?database={alias}&password={token}".format(proxy=proxy, alias=alias, token=token)
    try:
        resp = s.post(url, data=query, timeout=timeout)
        resp.raise_for_status()
    except HTTPError as err:
        if resp.text:
            raise HTTPError('{} Error Message: {}'.format(str(err.message), resp.text))
        else:
            raise err
    rows = resp.content.strip().split('\n')
    return rows


def chyt_execute_query(
    query,
    cluster,
    alias,
    token,
    columns,
    create_table_dict={}
):
    i = 0
    while True:
        try:
            if 'create table' in query.lower() or 'insert into' in query.lower():
                if create_table_dict:
                    delete_table(
                        create_table_dict['path'],
                        create_table_dict['tables_dir'],
                        create_table_dict['cluster'],
                        create_table_dict['cluster'].job()
                    )
                    execute_query(query=query, cluster=cluster, alias=alias, token=token)
                else:
                    execute_query(query=query, cluster=cluster, alias=alias, token=token)
                return 'Success!!!'
            else:
                result = execute_query(query=query, cluster=cluster, alias=alias, token=token)
                users = pd.DataFrame([row.split('\t') for row in result], columns=columns)
                return users
        except Exception as err:
            if 'IncompleteRead' in str(err.message):
                return 'Success!!!'
            print(err.message)
            i += 1
            time.sleep(5)
            if i > 30:
                print(query)
                raise ValueError('Bad Query!!!')


def wait_for_done_running_proccess(os, file_names):
    # lst = lst.split('\n')
    proccess_running = 0
    files_done_dict = {}
    file_names = file_names.split(',')
    while True:
        for file_name in file_names:
            lst = os.popen('ps -ef | grep python').read()
            if file_name in lst:
                print('%s is running' % (file_name))
                time.sleep(30)
            else:
                files_done_dict[file_name] = 1
                proccess_running = 0
                print('%s Done' % (file_name))

        if len(files_done_dict) == len(file_names):
            break


def get_last_not_empty_table(folder_path, job):
    tables_list = sorted([folder_path + '/' + x for x in job.driver.list(folder_path)], reverse=True)
    last_table_rows = 0
    last_table = ''
    for table in tables_list:
        try:
            table_rows = int(job.driver.get_attribute(table, 'chunk_row_count'))
        except:
            continue

        if table_rows > last_table_rows:
            last_table_rows = table_rows
            last_table = table
    if last_table:
        return last_table
    else:
        return tables_list[0]


def main():
    files_list = 'get_event_from_console_log.py'
    wait_for_done_running_proccess(os, files_list)

    cluster_yt = clusters.yt.Hahn(
        token=yt_creds['value']['token'],
        pool=yt_creds['value']['pool'],

    )
    job = cluster_yt.job()
    valid_path = '//home/cloud/billing/exported-billing-tables/billing_accounts_history_prod'
    cluster = 'hahn'
    alias = "*cloud_analytics"
    token = '%s' % (yt_creds['value']['token'])

    query = '''
    SELECT
        DISTINCT
        puid,
        first_first_trial_consumption_datetime as first_trial_consumption_datetime,
        multiIf(
            toDate(first_first_trial_consumption_datetime) <= toDate(addDays(NOW(), -70)), 'learning_set',
            'predict_test'
        ) as dataset_type,
        multiIf(
            first_first_paid_consumption_datetime < '2030-01-01 00:00:00' AND (ba_state != 'suspended' OR (ba_state = 'suspended' AND block_reason = 'trial_expired')),
            1,
            0
        ) as start_paid_consumption,
        multiIf(
            (ba_state != 'suspended' OR (ba_state = 'suspended' AND block_reason = 'trial_expired')),
            0,
            1
        ) as is_supended,
        multiIf(
            ba_state = 'suspended' AND block_reason IN ('manual', 'mining'),
            1,
            0
        ) as is_fraud
    FROM(
        SELECT
            puid,
            first_first_trial_consumption_datetime,
            ba_state,
            block_reason,
            addDays(toDateTime(event_time), 7) AS last_event_datetime,
            multiIf(first_first_paid_consumption_datetime IS NULL OR first_first_paid_consumption_datetime = '0000-00-00 00:00:00','2030-01-01 00:00:00',first_first_paid_consumption_datetime) as first_first_paid_consumption_datetime
        FROM
            "//home/cloud_analytics/cubes/acquisition_cube/cube"
        WHERE
            event = 'first_trial_consumption'
            AND toDate(event_time) > toDate('2018-12-10')
            AND toDate(event_time) <= toDate(addDays(NOW(), -8))
    )
    WHERE
        toDateTime(first_first_paid_consumption_datetime) > toDateTime(last_event_datetime)
    '''
    columns = ['puid', 'first_trial_consumption_datetime', 'dataset_type', 'start_paid_consumption', 'is_supended',
               'is_fraud']
    users = chyt_execute_query(query=query, cluster=cluster, alias=alias, token=token, columns=columns)

    int_cols = ['start_paid_consumption', 'is_supended', 'is_fraud']
    for col in int_cols:
        users[col] = users[col].astype(int)

    query = '''
    SELECT
        DISTINCT
        t0.puid,
        multiIf(start_compute < '2030-01-01 00:00:00'  AND (ba_state != 'suspended' OR (ba_state = 'suspended' AND block_reason = 'trial_expired')), 1,0) as start_compute,
        multiIf(start_mdb < '2030-01-01 00:00:00'  AND (ba_state != 'suspended' OR (ba_state = 'suspended' AND block_reason = 'trial_expired')), 1,0) as start_mdb,
        multiIf(start_storage < '2030-01-01 00:00:00'  AND (ba_state != 'suspended' OR (ba_state = 'suspended' AND block_reason = 'trial_expired')), 1,0) as start_storage,
        multiIf(start_ai < '2030-01-01 00:00:00'  AND (ba_state != 'suspended' OR (ba_state = 'suspended' AND block_reason = 'trial_expired')), 1,0) as start_ai
    FROM(
        SELECT
            puid,
            ba_state,
            block_reason,
            MAX(multiIf(service_name = 'compute', time, '2030-01-01 00:00:00')) as start_compute,
            MAX(multiIf(service_name = 'mdb', time, '2030-01-01 00:00:00')) as start_mdb,
            MAX(multiIf(service_name = 'storage', time, '2030-01-01 00:00:00')) as start_storage,
            MAX(multiIf(service_name = 'cloud_ai', time, '2030-01-01 00:00:00')) as start_ai
        FROM(
            SELECT
                puid,
                ba_state,
                block_reason,
                multiIf(service_name LIKE '%mdb%', 'mdb', service_name) as service_name,
                MIN(event_time) as time
            FROM
                "//home/cloud_analytics/cubes/acquisition_cube/cube"
            WHERE
                event = 'day_use'
                AND multiIf(service_name LIKE '%mdb%', 'mdb', service_name) IN ('compute', 'mdb', 'cloud_ai', 'storage')
                AND real_consumption > 0
            GROUP BY
                puid,
                ba_state,
                block_reason,
                service_name
        )
        GROUP BY
            puid,
            ba_state,
            block_reason
        ) as t0
    SEMI LEFT JOIN(
        SELECT
            puid
        FROM(
            SELECT
                puid,
                first_first_trial_consumption_datetime,
                addDays(toDateTime(event_time), 7) AS last_event_datetime,
                multiIf(first_first_paid_consumption_datetime IS NULL OR first_first_paid_consumption_datetime = '0000-00-00 00:00:00','2030-01-01 00:00:00',first_first_paid_consumption_datetime) as first_first_paid_consumption_datetime
            FROM
                "//home/cloud_analytics/cubes/acquisition_cube/cube"
            WHERE
                event = 'first_trial_consumption'
                AND toDate(event_time) > toDate('2018-12-10')
                AND toDate(event_time) <= toDate(addDays(NOW(), -8))
        )
        WHERE
            toDateTime(first_first_paid_consumption_datetime) > toDateTime(last_event_datetime)
    ) as t1
    ON t0.puid = t1.puid
    '''

    columns = ['puid', 'start_compute', 'start_mdb', 'start_storage', 'start_ai']

    users_services = chyt_execute_query(query=query, cluster=cluster, alias=alias, token=token, columns=columns)

    int_cols = ['start_compute', 'start_mdb', 'start_storage', 'start_ai']
    for col in int_cols:
        users_services[col] = users_services[col].astype(int)

    columns = [
        'puid',
        'all_paid_consumption',
        'all_trial_consumption_count',
        'all_trial_consumption_sum',
        'all_trial_consumption_avg',
        'all_trial_consumption_max',
        'all_trial_consumption_min',
        'all_trial_consumption_median',
        'all_trial_consumption_std',
        'all_trial_consumption_count_more_avg',
        'mdb_trial_consumption_count',
        'mdb_trial_consumption_sum',
        'mdb_trial_consumption_avg',
        'mdb_trial_consumption_max',
        'mdb_trial_consumption_min',
        'mdb_trial_consumption_median',
        'mdb_trial_consumption_std',
        'mdb_trial_consumption_count_more_avg',
        'ai_trial_consumption_count',
        'ai_trial_consumption_sum',
        'ai_trial_consumption_avg',
        'ai_trial_consumption_max',
        'ai_trial_consumption_min',
        'ai_trial_consumption_median',
        'ai_trial_consumption_std',
        'ai_trial_consumption_count_more_avg',
        'storage_trial_consumption_count',
        'storage_trial_consumption_sum',
        'storage_trial_consumption_avg',
        'storage_trial_consumption_max',
        'storage_trial_consumption_min',
        'storage_trial_consumption_median',
        'storage_trial_consumption_std',
        'storage_trial_consumption_count_more_avg',
        'network_trial_consumption_count',
        'network_trial_consumption_sum',
        'network_trial_consumption_avg',
        'network_trial_consumption_max',
        'network_trial_consumption_min',
        'network_trial_consumption_median',
        'network_trial_consumption_std',
        'network_trial_consumption_count_more_avg',
        'nlb_trial_consumption_count',
        'nlb_trial_consumption_sum',
        'nlb_trial_consumption_avg',
        'nlb_trial_consumption_max',
        'nlb_trial_consumption_min',
        'nlb_trial_consumption_median',
        'nlb_trial_consumption_std',
        'nlb_trial_consumption_count_more_avg',
        'marketplace_trial_consumption_count',
        'marketplace_trial_consumption_sum',
        'marketplace_trial_consumption_avg',
        'marketplace_trial_consumption_max',
        'marketplace_trial_consumption_min',
        'marketplace_trial_consumption_median',
        'marketplace_trial_consumption_std',
        'marketplace_trial_consumption_count_more_avg',
        'nbs_trial_consumption_count',
        'nbs_trial_consumption_sum',
        'nbs_trial_consumption_avg',
        'nbs_trial_consumption_max',
        'nbs_trial_consumption_min',
        'nbs_trial_consumption_median',
        'nbs_trial_consumption_std',
        'nbs_trial_consumption_count_more_avg',
        'snapshot_trial_consumption_count',
        'snapshot_trial_consumption_sum',
        'snapshot_trial_consumption_avg',
        'snapshot_trial_consumption_max',
        'snapshot_trial_consumption_min',
        'snapshot_trial_consumption_median',
        'snapshot_trial_consumption_std',
        'snapshot_trial_consumption_count_more_avg',
        'image_trial_consumption_count',
        'image_trial_consumption_sum',
        'image_trial_consumption_avg',
        'image_trial_consumption_max',
        'image_trial_consumption_min',
        'image_trial_consumption_median',
        'image_trial_consumption_std',
        'image_trial_consumption_count_more_avg'
    ]

    query = '''
    SELECT
        puid,
        SUM(all_paid_consumption) as all_paid_consumption,
        SUM(multiIf(all_trial_consumption > 0, 1,0)) as all_trial_consumption_count,
        SUM(all_trial_consumption) as all_trial_consumption_sum,
        AVG(all_trial_consumption) as all_trial_consumption_avg,
        MAX(all_trial_consumption) as all_trial_consumption_max,
        MIN(all_trial_consumption) as all_trial_consumption_min,
        median(all_trial_consumption) as all_trial_consumption_median,
        stddevPop(all_trial_consumption) as all_trial_consumption_std,
        arraySum(arrayMap(x -> x > all_trial_consumption_avg, groupArray(all_trial_consumption))) as all_trial_consumption_count_more_avg,
        SUM(multiIf(mdb_trial_consumption > 0, 1,0)) as mdb_trial_consumption_count,
        SUM(mdb_trial_consumption) as mdb_trial_consumption_sum,
        AVG(mdb_trial_consumption) as mdb_trial_consumption_avg,
        MAX(mdb_trial_consumption) as mdb_trial_consumption_max,
        MIN(mdb_trial_consumption) as mdb_trial_consumption_min,
        median(mdb_trial_consumption) as mdb_trial_consumption_median,
        stddevPop(mdb_trial_consumption) as mdb_trial_consumption_std,
        arraySum(arrayMap(x -> x > mdb_trial_consumption_avg, groupArray(mdb_trial_consumption))) as mdb_trial_consumption_count_more_avg,
        SUM(multiIf(ai_trial_consumption > 0, 1,0)) as ai_trial_consumption_count,
        SUM(ai_trial_consumption) as ai_trial_consumption_sum,
        AVG(ai_trial_consumption) as ai_trial_consumption_avg,
        MAX(ai_trial_consumption) as ai_trial_consumption_max,
        MIN(ai_trial_consumption) as ai_trial_consumption_min,
        median(ai_trial_consumption) as ai_trial_consumption_median,
        stddevPop(ai_trial_consumption) as ai_trial_consumption_std,
        arraySum(arrayMap(x -> x > ai_trial_consumption_avg, groupArray(ai_trial_consumption))) as ai_trial_consumption_count_more_avg,
        SUM(multiIf(storage_trial_consumption > 0, 1,0)) as storage_trial_consumption_count,
        SUM(storage_trial_consumption) as storage_trial_consumption_sum,
        AVG(storage_trial_consumption) as storage_trial_consumption_avg,
        MAX(storage_trial_consumption) as storage_trial_consumption_max,
        MIN(storage_trial_consumption) as storage_trial_consumption_min,
        median(storage_trial_consumption) as storage_trial_consumption_median,
        stddevPop(storage_trial_consumption) as storage_trial_consumption_std,
        arraySum(arrayMap(x -> x > storage_trial_consumption_avg, groupArray(storage_trial_consumption))) as storage_trial_consumption_count_more_avg,
        SUM(multiIf(network_trial_consumption > 0, 1,0)) as network_trial_consumption_count,
        SUM(network_trial_consumption) as network_trial_consumption_sum,
        AVG(network_trial_consumption) as network_trial_consumption_avg,
        MAX(network_trial_consumption) as network_trial_consumption_max,
        MIN(network_trial_consumption) as network_trial_consumption_min,
        median(network_trial_consumption) as network_trial_consumption_median,
        stddevPop(network_trial_consumption) as network_trial_consumption_std,
        arraySum(arrayMap(x -> x > network_trial_consumption_avg, groupArray(network_trial_consumption))) as network_trial_consumption_count_more_avg,
        SUM(multiIf(nlb_trial_consumption > 0, 1,0)) as nlb_trial_consumption_count,
        SUM(nlb_trial_consumption) as nlb_trial_consumption_sum,
        AVG(nlb_trial_consumption) as nlb_trial_consumption_avg,
        MAX(nlb_trial_consumption) as nlb_trial_consumption_max,
        MIN(nlb_trial_consumption) as nlb_trial_consumption_min,
        median(nlb_trial_consumption) as nlb_trial_consumption_median,
        stddevPop(nlb_trial_consumption) as nlb_trial_consumption_std,
        arraySum(arrayMap(x -> x > nlb_trial_consumption_avg, groupArray(nlb_trial_consumption))) as nlb_trial_consumption_count_more_avg,
        SUM(multiIf(marketplace_trial_consumption > 0, 1,0)) as marketplace_trial_consumption_count,
        SUM(marketplace_trial_consumption) as marketplace_trial_consumption_sum,
        AVG(marketplace_trial_consumption) as marketplace_trial_consumption_avg,
        MAX(marketplace_trial_consumption) as marketplace_trial_consumption_max,
        MIN(marketplace_trial_consumption) as marketplace_trial_consumption_min,
        median(marketplace_trial_consumption) as marketplace_trial_consumption_median,
        stddevPop(marketplace_trial_consumption) as marketplace_trial_consumption_std,
        arraySum(arrayMap(x -> x > marketplace_trial_consumption_avg, groupArray(marketplace_trial_consumption))) as marketplace_trial_consumption_count_more_avg,
        SUM(multiIf(nbs_trial_consumption > 0, 1,0)) as nbs_trial_consumption_count,
        SUM(nbs_trial_consumption) as nbs_trial_consumption_sum,
        AVG(nbs_trial_consumption) as nbs_trial_consumption_avg,
        MAX(nbs_trial_consumption) as nbs_trial_consumption_max,
        MIN(nbs_trial_consumption) as nbs_trial_consumption_min,
        median(nbs_trial_consumption) as nbs_trial_consumption_median,
        stddevPop(nbs_trial_consumption) as nbs_trial_consumption_std,
        arraySum(arrayMap(x -> x > nbs_trial_consumption_avg, groupArray(nbs_trial_consumption))) as nbs_trial_consumption_count_more_avg,
        SUM(multiIf(snapshot_trial_consumption > 0, 1,0)) as snapshot_trial_consumption_count,
        SUM(snapshot_trial_consumption) as snapshot_trial_consumption_sum,
        AVG(snapshot_trial_consumption) as snapshot_trial_consumption_avg,
        MAX(snapshot_trial_consumption) as snapshot_trial_consumption_max,
        MIN(snapshot_trial_consumption) as snapshot_trial_consumption_min,
        median(snapshot_trial_consumption) as snapshot_trial_consumption_median,
        stddevPop(snapshot_trial_consumption) as snapshot_trial_consumption_std,
        arraySum(arrayMap(x -> x > snapshot_trial_consumption_avg, groupArray(snapshot_trial_consumption))) as snapshot_trial_consumption_count_more_avg,
        SUM(multiIf(image_trial_consumption > 0, 1,0)) as image_trial_consumption_count,
        SUM(image_trial_consumption) as image_trial_consumption_sum,
        AVG(image_trial_consumption) as image_trial_consumption_avg,
        MAX(image_trial_consumption) as image_trial_consumption_max,
        MIN(image_trial_consumption) as image_trial_consumption_min,
        median(image_trial_consumption) as image_trial_consumption_median,
        stddevPop(image_trial_consumption) as image_trial_consumption_std,
        arraySum(arrayMap(x -> x > image_trial_consumption_avg, groupArray(image_trial_consumption))) as image_trial_consumption_count_more_avg
    FROM(
        SELECT
            *
        FROM(
            SELECT
                t0.puid,
                toDate(event_time) as date,
                assumeNotNull(SUM(trial_consumption)) as all_trial_consumption,
                assumeNotNull(SUM(real_consumption)) as all_paid_consumption,
                assumeNotNull(SUM(multiIf(service_name LIKE '%compute%', trial_consumption, 0))) as compute_trial_consumption,
                assumeNotNull(SUM(multiIf(service_name LIKE '%mdb%', trial_consumption, 0))) as mdb_trial_consumption,
                assumeNotNull(SUM(multiIf(service_name LIKE '%_ai%', trial_consumption, 0))) as ai_trial_consumption,
                assumeNotNull(SUM(multiIf(service_name LIKE '%storage%', trial_consumption, 0))) as storage_trial_consumption,
                assumeNotNull(SUM(multiIf(service_name LIKE '%network%', trial_consumption, 0))) as network_trial_consumption,
                assumeNotNull(SUM(multiIf(service_name LIKE '%nlb%', trial_consumption, 0))) as nlb_trial_consumption,
                assumeNotNull(SUM(multiIf(sku_name LIKE '%marketplace%', trial_consumption, 0))) as marketplace_trial_consumption,
                assumeNotNull(SUM(multiIf(sku_name LIKE '%nbs.%', trial_consumption, 0))) as nbs_trial_consumption,
                assumeNotNull(SUM(multiIf(sku_name LIKE '%snapshot%', trial_consumption, 0))) as snapshot_trial_consumption,
                assumeNotNull(SUM(multiIf(sku_name LIKE '%image%', trial_consumption, 0))) as image_trial_consumption
            FROM
                "//home/cloud_analytics/cubes/acquisition_cube/cube" as t0
            WHERE
                event = 'day_use'
            GROUP BY
                puid,
                date
        ) as t0
        SEMI LEFT JOIN (
            SELECT
                *
            FROM(
                SELECT
                    puid,
                    first_first_trial_consumption_datetime,
                    addDays(toDateTime(event_time), 7) AS last_event_datetime,
                    multiIf(first_first_paid_consumption_datetime IS NULL OR first_first_paid_consumption_datetime = '0000-00-00 00:00:00','2030-01-01 00:00:00',first_first_paid_consumption_datetime) as first_first_paid_consumption_datetime
                FROM
                    "//home/cloud_analytics/cubes/acquisition_cube/cube"
                WHERE
                    event = 'first_trial_consumption'
                    AND toDate(event_time) > toDate('2018-12-10')
                    AND toDate(event_time) <= toDate(addDays(NOW(), -8))
            )
            WHERE
                toDateTime(first_first_paid_consumption_datetime) > toDateTime(last_event_datetime)
        ) as t1
        ON t0.puid = t1.puid
        WHERE
             t0.date < toDate(t1.last_event_datetime)
        ORDER BY
            puid,
            date
    )
    GROUP BY
        puid
    '''
    cunsumption_stat = chyt_execute_query(query=query, cluster=cluster, alias=alias, token=token, columns=columns)

    float_cols = [
        'all_paid_consumption',
        'all_trial_consumption_count',
        'all_trial_consumption_sum',
        'all_trial_consumption_avg',
        'all_trial_consumption_max',
        'all_trial_consumption_min',
        'all_trial_consumption_median',
        'all_trial_consumption_std',
        'all_trial_consumption_count_more_avg',
        'mdb_trial_consumption_count',
        'mdb_trial_consumption_sum',
        'mdb_trial_consumption_avg',
        'mdb_trial_consumption_max',
        'mdb_trial_consumption_min',
        'mdb_trial_consumption_median',
        'mdb_trial_consumption_std',
        'mdb_trial_consumption_count_more_avg',
        'ai_trial_consumption_count',
        'ai_trial_consumption_sum',
        'ai_trial_consumption_avg',
        'ai_trial_consumption_max',
        'ai_trial_consumption_min',
        'ai_trial_consumption_median',
        'ai_trial_consumption_std',
        'ai_trial_consumption_count_more_avg',
        'storage_trial_consumption_count',
        'storage_trial_consumption_sum',
        'storage_trial_consumption_avg',
        'storage_trial_consumption_max',
        'storage_trial_consumption_min',
        'storage_trial_consumption_median',
        'storage_trial_consumption_std',
        'storage_trial_consumption_count_more_avg',
        'network_trial_consumption_count',
        'network_trial_consumption_sum',
        'network_trial_consumption_avg',
        'network_trial_consumption_max',
        'network_trial_consumption_min',
        'network_trial_consumption_median',
        'network_trial_consumption_std',
        'network_trial_consumption_count_more_avg',
        'nlb_trial_consumption_count',
        'nlb_trial_consumption_sum',
        'nlb_trial_consumption_avg',
        'nlb_trial_consumption_max',
        'nlb_trial_consumption_min',
        'nlb_trial_consumption_median',
        'nlb_trial_consumption_std',
        'nlb_trial_consumption_count_more_avg',
        'marketplace_trial_consumption_count',
        'marketplace_trial_consumption_sum',
        'marketplace_trial_consumption_avg',
        'marketplace_trial_consumption_max',
        'marketplace_trial_consumption_min',
        'marketplace_trial_consumption_median',
        'marketplace_trial_consumption_std',
        'marketplace_trial_consumption_count_more_avg',
        'nbs_trial_consumption_count',
        'nbs_trial_consumption_sum',
        'nbs_trial_consumption_avg',
        'nbs_trial_consumption_max',
        'nbs_trial_consumption_min',
        'nbs_trial_consumption_median',
        'nbs_trial_consumption_std',
        'nbs_trial_consumption_count_more_avg',
        'snapshot_trial_consumption_count',
        'snapshot_trial_consumption_sum',
        'snapshot_trial_consumption_avg',
        'snapshot_trial_consumption_max',
        'snapshot_trial_consumption_min',
        'snapshot_trial_consumption_median',
        'snapshot_trial_consumption_std',
        'snapshot_trial_consumption_count_more_avg',
        'image_trial_consumption_count',
        'image_trial_consumption_sum',
        'image_trial_consumption_avg',
        'image_trial_consumption_max',
        'image_trial_consumption_min',
        'image_trial_consumption_median',
        'image_trial_consumption_std',
        'image_trial_consumption_count_more_avg'
    ]
    for col in float_cols:
        cunsumption_stat[col] = cunsumption_stat[col].astype(float)

    query = '''
    SELECT
        DISTINCT
        t0.puid,
        multiIf(unreachible_count = calls, 0, 1) as is_reachible
    FROM(
        SELECT
            puid,
            groupArray(event_time) as event_times,
            groupArray(call_status) as call_statuses,
            arraySum(arrayMap(x -> x LIKE '%unreachible%', call_statuses)) as unreachible_count,
            arrayCount(arrayMap(x -> x IS NOT NULL, call_statuses)) as calls,
            event_times[1] as first_call_dt
        FROM(
            SELECT
                *
            FROM
                "//home/cloud_analytics_test/cubes/crm_leads/cube"
            WHERE
                event = 'call'
                AND puid != ''
                AND puid != '0'
            ORDER BY
                puid,
                event_time
        )
        GROUP BY
            puid
    ) as t0
    SEMI LEFT JOIN (
        SELECT
            *
        FROM(
            SELECT
                puid,
                first_first_trial_consumption_datetime,
                addDays(toDateTime(event_time), 7) AS last_event_datetime,
                multiIf(first_first_paid_consumption_datetime IS NULL OR first_first_paid_consumption_datetime = '0000-00-00 00:00:00','2030-01-01 00:00:00',first_first_paid_consumption_datetime) as first_first_paid_consumption_datetime
            FROM
                "//home/cloud_analytics/cubes/acquisition_cube/cube"
            WHERE
                event = 'first_trial_consumption'
                AND toDate(event_time) > toDate('2018-12-10')
                AND toDate(event_time) <= toDate(addDays(NOW(), -8))
        )
        WHERE
            toDateTime(first_first_paid_consumption_datetime) > toDateTime(last_event_datetime)
    ) as t1
    ON t0.puid = t1.puid
    WHERE
         toDate(t0.first_call_dt) > toDate(t1.last_event_datetime)
    '''
    columns = ['puid', 'is_reachible']
    calls = chyt_execute_query(query=query, cluster=cluster, alias=alias, token=token, columns=columns)

    calls['is_reachible'] = calls['is_reachible'].astype(int)

    columns = [
        'puid',
        'segment',
        'is_yandex_email',
        'is_corporate_email',
        'mobile_phone_vendor',
        'device_type',
        'days_between_first_visit_cloud',
        'days_between_cloud_ba',
        'hits',
        'os',
        'is_robot',
        'total_visits',
        'interests',
        'sex',
        'age',
        'session_start_time',
        'ad_block',
        'country',
        'search_phrase',
        'visit_version',
        'income',
        'channel',
        'promocode_source',
        'resolution_width',
        'resolution_height',
        'size_cat'
    ]

    query = '''
    SELECT
        t0.*
    FROM(
        SELECT
            puid,
            segment,
            multiIf(email LIKE '%@yandex.%' OR email LIKE '%@ya.%', 1, 0) AS is_yandex_email,
            multiIf(match(email,'.*@yandex\..*|.*@ya\..*|.*@gmail\..*|.*@mail\..*|.*@tut\..*|.*@linqcorp\..*'), 0, 1) AS is_corporate_email,
            multiIf(mobile_phone_vendor IS NULL, -1, mobile_phone_vendor) as mobile_phone_vendor,
            multiIf(device_type = '' OR device_type IS NULL, 'unknown', device_type) as device_type,
            0 as days_between_first_visit_cloud,
            0 as days_between_cloud_ba,
            multiIf(hits IS NULL, -1, hits) as hits,
            lowerUTF8(multiIf(os = '' OR os IS NULL, 'unknown', os)) as os,
            CAST(multiIf(is_robot = '' OR is_robot IS NULL, '-1', is_robot) as Int32) as is_robot,
            multiIf(total_visits IS NULL, -1, total_visits) as total_visits,
            CAST(multiIf(interests = '' OR interests IS NULL, '-1', interests) as Int32) as interests,
            multiIf(sex = '' OR sex IS NULL, 'unknown', sex) as sex,
            multiIf(age = '' OR age IS NULL, 'unknown', age) as age,
            session_start_time,
            multiIf(ad_block IS NULL, -1, ad_block) as ad_block,
            lowerUTF8(multiIf(country = '', 'unknown', country)) as country,
            lowerUTF8(multiIf(search_phrase = '', 'unknown', search_phrase)) as search_phrase,
            CAST(0 as Int32) as visit_version,
            multiIf(income IS NULL, -1, income) as income,
            channel,
            'unlnown' as promocode_source,
            multiIf(resolution_width IS NULL, -1, resolution_width) as resolution_width,
            multiIf(resolution_height IS NULL, -1, resolution_height) as resolution_height,
            multiIf( resolution_height > 0, resolution_width/resolution_height, 0) as size_cat
        FROM
            "//home/cloud_analytics/cubes/acquisition_cube/cube"
        WHERE
            event = 'cloud_created'
    ) as t0
    SEMI LEFT JOIN (
        SELECT
            *
        FROM(
            SELECT
                puid,
                first_first_trial_consumption_datetime,
                addDays(toDateTime(event_time), 7) AS last_event_datetime,
                multiIf(first_first_paid_consumption_datetime IS NULL OR first_first_paid_consumption_datetime = '0000-00-00 00:00:00','2030-01-01 00:00:00',first_first_paid_consumption_datetime) as first_first_paid_consumption_datetime
            FROM
                "//home/cloud_analytics/cubes/acquisition_cube/cube"
            WHERE
                event = 'first_trial_consumption'
                AND toDate(event_time) > toDate('2018-12-10')
                AND toDate(event_time) <= toDate(addDays(NOW(), -8))
        )
        WHERE
            toDateTime(first_first_paid_consumption_datetime) > toDateTime(last_event_datetime)
    ) as t1
    ON t0.puid = t1.puid
    '''

    user_meta_info = chyt_execute_query(query=query, cluster=cluster, alias=alias, token=token, columns=columns)

    int_cols = [
        'is_yandex_email',
        'mobile_phone_vendor',
        'hits',
        'is_robot',
        'total_visits',
        'interests',
        'ad_block',
        'visit_version',
        'income',
        'resolution_width',
        'resolution_height',
        'is_corporate_email',
        'days_between_first_visit_cloud',
        'days_between_cloud_ba'
    ]
    float_cols = [
        'size_cat'
    ]
    for col in int_cols:
        user_meta_info[col] = user_meta_info[col].astype(int)

    for col in float_cols:
        user_meta_info[col] = user_meta_info[col].astype(float)

    query = '''
    SELECT
        puid as puid,
        groupArray(payment_cycle_type)[1] as ba_payment_cycle_type,
        groupArray(state)[1] as ba_state,
        groupArray(person_type)[1] as ba_person_type,
        groupArray(payment_type)[1] as ba_payment_type,
        groupArray(usage_status)[1] as ba_usage_status,
        groupArray(type)[1] as ba_type
    FROM (
        SELECT
            t0.*,
            t1.puid,
            t1.last_event_datetime as last_event_datetime
        FROM(
            SELECT
                toDateTime(updated_at) as datetime,
                *
            FROM
                "{0}"
        ) as t0
        SEMI LEFT JOIN (
            SELECT
                *
            FROM(
                SELECT
                    puid,
                    billing_account_id,
                    first_first_trial_consumption_datetime,
                    addDays(toDateTime(event_time), 7) AS last_event_datetime,
                    multiIf(first_first_paid_consumption_datetime IS NULL OR first_first_paid_consumption_datetime = '0000-00-00 00:00:00','2030-01-01 00:00:00',first_first_paid_consumption_datetime) as first_first_paid_consumption_datetime
                FROM
                    "//home/cloud_analytics/cubes/acquisition_cube/cube"
                WHERE
                    event = 'first_trial_consumption'
                    AND puid != ''
                    AND toDate(event_time) > toDate('2018-12-10')
                    AND toDate(event_time) <= toDate(addDays(NOW(), -8))
            )
            WHERE
                toDateTime(first_first_paid_consumption_datetime) > toDateTime(last_event_datetime)
        ) as t1
        ON t0.billing_account_id = t1.billing_account_id
        WHERE
            toDate(t0.datetime) < toDate(t1.last_event_datetime)
        ORDER BY
            puid,
            datetime DESC
    )
    GROUP BY
        puid
    '''.format(valid_path)

    columns = ['puid', 'ba_payment_cycle_type', 'ba_state', 'ba_person_type', 'ba_payment_type', 'ba_usage_status',
               'ba_type']

    ba_meta_info = chyt_execute_query(query=query, cluster=cluster, alias=alias, token=token, columns=columns)

    query = '''
    SELECT
        DISTINCT
        puid,
        is_see_in_metriks
    FROM(
        SELECT
            puid,
            1 as is_see_in_metriks
        FROM(
            SELECT
                puid,
                splitByString(' ', assumeNotNull(event_time))[1] as date
            FROM
                "//home/cloud_analytics/cubes/acquisition_cube/cube"
            WHERE
                puid != ''
                AND event = 'visit'
        ) as t0
        SEMI LEFT JOIN (
            SELECT
                *
            FROM(
                SELECT
                    DISTINCT puid,
                    first_first_trial_consumption_datetime,
                    addDays(toDateTime(event_time), 7) AS last_event_datetime,
                    multiIf(first_first_paid_consumption_datetime IS NULL OR first_first_paid_consumption_datetime = '0000-00-00 00:00:00','2030-01-01 00:00:00',first_first_paid_consumption_datetime) as first_first_paid_consumption_datetime
                FROM
                    "//home/cloud_analytics/cubes/acquisition_cube/cube"
                WHERE
                    event = 'first_trial_consumption'
                    AND toDate(event_time) > toDate('2018-12-10')
                    AND toDate(event_time) <= toDate(addDays(NOW(), -8))
            )
            WHERE
                toDateTime(first_first_paid_consumption_datetime) > toDateTime(last_event_datetime)
        ) as t1
        ON t0.puid = t1.puid
        WHERE
            toDate(t0.date) < toDate(t1.last_event_datetime)
    )
    '''

    columns = ['puid', 'is_see_in_metriks']
    metrika_site_events = chyt_execute_query(query=query, cluster=cluster, alias=alias, token=token, columns=columns)

    metrika_site_events['is_see_in_metriks'] = metrika_site_events['is_see_in_metriks'].astype(int)

    targets = pd.merge(
        users,
        users_services,
        on='puid',
        how='left'
    ).fillna(0)

    targets = pd.merge(
        targets,
        calls,
        on='puid',
        how='left'
    ).fillna(-1)
    targets

    cluster_yt.write('//home/cloud_analytics/scoring/targets', targets)

    data = pd.merge(
        users[['puid']],
        cunsumption_stat.drop('all_paid_consumption', axis=1),
        on='puid',
        how='left'
    ).fillna(0)

    data = pd.merge(
        data,
        user_meta_info,
        on='puid',
        how='left'
    ).fillna(0)
    data = pd.merge(
        data,
        ba_meta_info,
        on='puid',
        how='left'
    ).fillna('unknown')
    data = pd.merge(
        data,
        metrika_site_events,
        on='puid',
        how='left'
    ).fillna(0)

    cluster_yt.write('//home/cloud_analytics/scoring/meta_info', data)

    query = '''
SELECT
    t0.*,
    runningDifference(t0.ts) as delta
FROM(
    SELECT
        puid,
        event_type,
        event,
        timestamp,
        ts,
        splitByString('T', assumeNotNull(timestamp))[1] as date
    FROM
        "//home/cloud_analytics/import/console_logs/events"
    WHERE
        puid != ''
    ORDER BY
        puid,
        timestamp
) as t0
SEMI LEFT JOIN (
    SELECT
        *
    FROM(
        SELECT
            puid,
            first_first_trial_consumption_datetime,
            addDays(toDateTime(event_time), 7) AS last_event_datetime,
            multiIf(first_first_paid_consumption_datetime IS NULL OR first_first_paid_consumption_datetime = '0000-00-00 00:00:00','2030-01-01 00:00:00',first_first_paid_consumption_datetime) as first_first_paid_consumption_datetime
        FROM
            "//home/cloud_analytics/cubes/acquisition_cube/cube"
        WHERE
            event = 'first_trial_consumption'
            AND toDate(event_time) > toDate('2018-12-10')
            AND toDate(event_time) <= toDate(addDays(NOW(), -8))
    ) as sub
    WHERE
        toDateTime(sub.first_first_paid_consumption_datetime) > toDateTime(sub.last_event_datetime)
) as t1
ON t0.puid = t1.puid
WHERE
    toDate(t0.date) < toDate(t1.last_event_datetime)
    '''

    columns = ['puid', 'event_type', 'event', 'timestamp', 'ts', 'date', 'delta']
    site_events = chyt_execute_query(query=query, cluster=cluster, alias=alias, token=token, columns=columns)
    cluster_yt.write('//home/cloud_analytics/scoring/events', site_events)


if __name__ == '__main__':
    main()
