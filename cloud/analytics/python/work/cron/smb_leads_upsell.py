#!/usr/bin/env python2
# -*- coding: utf-8 -*-
import pandas as pd, datetime, ast, numpy as np, telebot, json, requests, os, sys, pymysql,time
module_path = os.path.abspath(os.path.join('/home/ktereshin/yandex/arcadia/cloud/analytics/python/work'))
if module_path not in sys.path:
    sys.path.append(module_path)
module_path = os.path.abspath(os.path.join('/home/ktereshin/yandex/cron'))
if module_path not in sys.path:
    sys.path.append(module_path)
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
    stat_creds,
    telebot_creds
)

from acquisition_cube_init_funnel_steps import (
    paths_dict_prod,
    paths_dict_test
)

def delete_table(path_, tables_dir_, cluster_yt_, job_):
    if path_ in get_table_list(tables_dir_, job_)[1:-1].split(','):
        counter = 0
        while True:
            time.sleep(2)
            try:
                cluster_yt_.driver.remove(path_)
                break
            except Exception as err:
                print(err)
                counter += 1
                if counter == 15:
                    break

def convert_metric_to_float(num):
    try:
        return float(num)
    except:
        return 0.0


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
            last_table_rows =  table_rows
            last_table = table
    if last_table:
        return last_table
    else:
        return tables_list[0]


def get_table_list(folder_path, job):
    tables_list = sorted([folder_path + '/' + x for x in job.driver.list(folder_path)], reverse=True)
    return '{%s}' % (','.join(tables_list))

def apply_types_in_project(schema_):
    apply_types_dict = {}
    for col in schema_:

        if schema_[col] == str:
            apply_types_dict[col] = ne.custom(lambda x: str(x).replace('"', '').replace("'", '').replace('\\','') if x not in ['', None] else None, col)

        elif schema_[col] == int:
            apply_types_dict[col] = ne.custom(lambda x: int(x) if x not in ['', None] else None, col)

        elif schema_[col] == float:
            apply_types_dict[col] = ne.custom(lambda x: float(x) if x not in ['', None] else None, col)
    return apply_types_dict

def execute_query(query, cluster, alias, token, timeout=1200):
    proxy = "http://{}.yt.yandex.net".format(cluster)
    s = requests.Session()
    url = "{proxy}/query?database={alias}&password={token}".format(proxy=proxy, alias=alias, token=token)
    resp = s.post(url, data=query, timeout=timeout)
    resp.raise_for_status()
    rows = resp.content.strip().split('\n')
    return rows

def chyt_execute_query(
    query,
    cluster,
    alias,
    token,
    columns,
    create_table_dict = {}
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
                users = pd.DataFrame([row.split('\t') for row in result], columns = columns)
                return users
        except Exception as err:
            if 'IncompleteRead' in  str(err.message):
                return 'Success!!!'
            print(err.message)
            i += 1
            time.sleep(5)
            if i > 30:
                print(query)
                raise ValueError('Bad Query!!!')

def delete_table(path_, tables_dir_, cluster_yt_, job_):
    if path_ in get_table_list(tables_dir_, job_)[1:-1].split(','):
        counter = 0
        while True:
            time.sleep(2)
            try:
                cluster_yt_.driver.remove(path_)
                break
            except Exception as err:
                print(err)
                counter += 1
                if counter == 15:
                    break
def get_bd(port):
    if port in [6432, 5432]:
        return 'postgresql'
    if port == 3306:
        return 'mysql'
    if port == 6371:
        return 'redis'
    if port in [9000,8123,8443]:
        return 'clickhouse'
    return 'other'

def get_date(date_):
    return str(date_).split('T')[0]

def main():
    cluster = 'hahn'
    alias = "*cloud_analytics"
    #alias = "*ch_public"
    token = '%s' % (yt_creds['value']['token'])

    mode = 'prod'
    bot = telebot.TeleBot(telebot_creds['value']['token'])
    cluster_yt = clusters.yt.Hahn(
        token = yt_creds['value']['token'],
        pool = yt_creds['value']['pool']
    )

    cnx = pymysql.connect(
        user='cloud8',
        password=crm_sql_creds['value']['readonly'],
        host='c-mdb8t5pqa6cptk82ukmc.rw.db.yandex.net',
        port = 3306,
        database='cloud8'
    )
    query = '''
    SELECT
        ba_id as billing_account_id,
        MAX(phone) as phone,
        MAX(email) as email
    FROM(
        (SELECT
            d.name AS dimen_name,
            bean.bean_module,
            l.id AS bean_id,
            CONCAT(l.first_name, ' ', l.last_name) AS name,
            ba.name AS ba_id,
            l.phone_mobile as phone,
            l1.email_address as email
        FROM
            cloud8.dimensions AS d
        INNER JOIN cloud8.dimensions_bean_rel AS bean
            ON d.id = bean.dimension_id
        LEFT JOIN cloud8.leads AS l
            ON bean.bean_id = l.id
        LEFT JOIN  email_addr_bean_rel l1_1
            ON l.id=l1_1.bean_id AND l1_1.deleted=0 AND l1_1.bean_module = 'Leads' AND l1_1.primary_address = 1
        INNER JOIN  email_addresses l1
            ON l1.id=l1_1.email_address_id
        LEFT JOIN cloud8.leads_billing_accounts AS l_bean
            ON l_bean.leads_id = l.id
        LEFT JOIN cloud8.billingaccounts AS ba
            ON l_bean.billingaccounts_id = ba.id
        WHERE
            d.name = 'Business'
                AND bean.bean_module = 'Leads'
                AND l.deleted = 0
                AND ba.name IS NOT NULL)
        UNION ALL
        (SELECT
            d.name AS dimen_name,
            bean.bean_module,
            acc.id AS acc_id,
            acc.name AS acc_name,
            ba.name AS ba_id,
            cont.phone_mobile as phone,
            l1.email_address as email
        FROM
            cloud8.dimensions AS d
        INNER JOIN cloud8.dimensions_bean_rel AS bean
            ON d.id = bean.dimension_id
        LEFT JOIN cloud8.accounts AS acc
            ON bean.bean_id = acc.id
        LEFT JOIN  email_addr_bean_rel l1_1
            ON acc.id=l1_1.bean_id AND l1_1.deleted=0 AND l1_1.bean_module = 'Accounts' AND l1_1.primary_address = 1
        LEFT JOIN  email_addresses l1
            ON l1.id=l1_1.email_address_id
        LEFT JOIN cloud8.accounts_contacts AS acc_cont
            ON acc.id = acc_cont.account_id
        LEFT JOIN cloud8.contacts AS cont
            ON cont.id = acc_cont.contact_id
        LEFT JOIN cloud8.billingaccounts AS ba
            ON ba.account_id = acc.id
        WHERE
            d.name = 'Business'
            AND bean.bean_module = 'Accounts'
            AND acc.deleted = 0
            AND ba.name IS NOT NULL)
    ) as t0
    GROUP BY
        ba_id
    '''

    ba_bus = pd.read_sql_query(query, cnx)
    cnx.close()

    cluster_yt.write('//home/cloud_analytics/import/crm/business_accounts/data' + '_', ba_bus)

    schema = {
        "billing_account_id": str,
        'phone': str,
        'email': str
    }
    job = cluster_yt.job()
    job.table('//home/cloud_analytics/import/crm/business_accounts/data' + '_') \
    .project(
        **apply_types_in_project(schema)
    ) \
    .put('//home/cloud_analytics/import/crm/business_accounts/data', schema = schema)
    job.run()
    cluster_yt.driver.remove('//home/cloud_analytics/import/crm/business_accounts/data' + '_')

    '''
    last_day = str(datetime.date.today()-datetime.timedelta(days = 1))
    cluster_yt = clusters.yt.Hahn(
        token = yt_creds['value']['token'],
        pool = yt_creds['value']['pool']
    ).env(
        templates=dict(
            dates='{2019-12-04..2020-01-17}' #% (last_day)
        )
    )
    schema = {
        'billing_account_id': str,
        'db': str,
        'date': str
    }
    job = cluster_yt.job()
    db = job.table('//logs/yc-antifraud-overlay-flows-stats/1d/@dates') \
    .filter(
        nf.custom(lambda x: x in [6432, 5432, 3306, 6371, 9000, 8123, 8443], 'sport')
    ) \
    .project(
        'cloud_id',
        db = ne.custom(get_bd, 'sport'),
        date = ne.custom(lambda x: str(x).split('T')[0], 'setup_time')
    ) \
    .unique(
        'cloud_id',
        'db',
        'date'
    )

    dict_ = job.table('//home/cloud_analytics/dictionaries/ids/ba_cloud_folder_dict') \
    .filter(
        nf.custom(lambda x: x not in [None, ''], 'billing_account_id')
    ) \
    .groupby(
        'cloud_id'
    ) \
    .aggregate(
        billing_account_id = na.last('billing_account_id', by='ts')
    )

    db = db.join(
        dict_,
        by = 'cloud_id',
        type = 'left'
    ) \
    .unique(
        'billing_account_id',
        'db',
        'date'
    ) \
    .project(
        **apply_types_in_project(schema)
    ) \
    .put('//home/cloud_analytics/import/network-logs/db-on-vm/data', schema = schema)
    job.run()
    '''

    last_day = str(datetime.date.today()-datetime.timedelta(days = 1))

    cluster_yt = clusters.yt.Hahn(
        token = yt_creds['value']['token'],
        pool = yt_creds['value']['pool']
    ).env(
        templates=dict(
            dates='{%s}' % (last_day)
        )
    )

    schema = {
        'billing_account_id': str,
        'db': str,
        'date': str
    }
    while True:
        try:
            job = cluster_yt.job()
            source = job.table('//home/cloud_analytics/import/network-logs/db-on-vm/data') \
            .filter(
                nf.custom(lambda x: x < last_day, 'date')
            )

            db = job.table('//logs/yc-antifraud-overlay-flows-stats/1d/@dates') \
            .filter(
                nf.custom(lambda x: x in [6432, 5432, 3306, 6371, 9000, 8123, 8443], 'sport'),
                nf.custom(lambda x: x >= last_day, 'setup_time')
            ) \
            .project(
                'cloud_id',
                db = ne.custom(get_bd, 'sport'),
                date = ne.custom(lambda x: str(x).split('T')[0], 'setup_time')
            ) \
            .unique(
                'cloud_id',
                'db',
                'date'
            )

            dict_ = job.table('//home/cloud_analytics/dictionaries/ids/ba_cloud_folder_dict') \
            .filter(
                nf.custom(lambda x: x not in [None, ''], 'billing_account_id')
            ) \
            .groupby(
                'cloud_id'
            ) \
            .aggregate(
                billing_account_id = na.last('billing_account_id', by='ts')
            )

            db = db.join(
                dict_,
                by = 'cloud_id',
                type = 'left'
            ) \
            .unique(
                'billing_account_id',
                'db',
                'date'
            )

            job.concat(
                source,
                db
            ) \
            .project(
                **apply_types_in_project(schema)
            ) \
            .put('//home/cloud_analytics/import/network-logs/db-on-vm/data', schema = schema)
            job.run()
            break
        except:
            time.sleep(300)

    if mode == 'test':
        paths_dict_temp = paths_dict_test
        folder = 'cloud_analytics_test'
        tables_dir = "//home/{0}/smb/upsell".format(folder)
    elif mode == 'prod':
        paths_dict_temp = paths_dict_prod
        folder = 'cloud_analytics'
        tables_dir = "//home/{0}/smb/upsell".format(folder)
    paths_dict = paths_dict_temp.copy()

    query = '''
    CREATE TABLE "//home/{0}/smb/upsell/billing_accounts" ENGINE=YtTable() AS
    --INSERT INTO "<append=%false>//home/{0}/smb/upsell/billing_accounts"
    SELECT
        *
    FROM(
        SELECT
            result.*,
            multiIf(net_data.total_paid > 0, net_data.total_paid, 0) as total_paid,
            multiIf(net_data.ai_paid > 0, net_data.ai_paid, 0) as ai_paid,
            multiIf(net_data.mdb_paid > 0, net_data.mdb_paid, 0) as mdb_paid,
            multiIf(net_data.data_bases != '', net_data.data_bases, 'have not DB on VM') as data_bases
        FROM(
        SELECT
            res.*,
            multiIf(tm.timezone IS NULL, 'Europe/Moscow', tm.timezone) as timezone
        FROM(
            SELECT
                main_call.*,
                multiIf(con.paid_avg > 0, con.paid_avg, 0) as paid_avg,
                multiIf(con.paid_std > 0, con.paid_std, 0) as paid_std
            FROM(
                SELECT
                    billing_account_id,
                    puid,
                    ba_created_datetime,
                    email,
                    phone,
                    first_name,
                    last_name,
                    client_name,
                    source,
                    multiIf(last_call_time IS NULL OR last_call_time = '', ba_created_datetime, last_call_time) as last_call_time,
                    multiIf(last_source IS NULL OR last_source = '', 'unknown', last_source) as last_source
                FROM(
                SELECT
                    billing_account_id,
                    puid,
                    multiIf(
                        t0.email IS NOT NULL AND t0.email != '', t0.email,
                        t1.email IS NOT NULL AND t1.email != '', t1.email,
                        NULL
                    ) as email,
                    multiIf(
                        t0.phone IS NOT NULL AND t0.phone != '', t0.phone,
                        t1.phone IS NOT NULL AND t1.phone != '', t1.phone,
                        NULL
                    ) as phone,
                    first_name,
                    last_name,
                    client_name,
                    'upsell' as source,
                    ba_created_datetime
                FROM(
                    SELECT
                        billing_account_id,
                        puid,
                        multiIf(
                            user_settings_email LIKE '%@yandex.%' OR user_settings_email LIKE '%@ya.%',CONCAT(lower(replaceAll(splitByString('@',assumeNotNull(user_settings_email))[1], '.', '-')), '@yandex.ru'),
                            lower(user_settings_email)
                        ) as email,
                        phone,
                        multiIf(first_name IS NULL OR first_name = '', 'unknown', first_name) as first_name,
                        multiIf(last_name IS NULL OR last_name = '', 'unknown', last_name) as last_name,
                        multiIf(account_name IS NULL OR account_name = '', 'unknown', account_name) as client_name,
                        ba_person_type,
                        is_corporate_card,
                        event_time as ba_created_datetime
                    FROM
                        "//home/cloud_analytics/cubes/acquisition_cube/cube"
                    WHERE
                        event = 'ba_created'
                        AND block_reason NOT IN ('manual', 'mining')
                        AND segment NOT IN ('Large ISV', 'Medium', 'Enterprise')
                        AND ba_usage_status != 'service'
                        AND is_fraud = 0
                ) as t0
                ANY LEFT JOIN(
                    SELECT
                        *,
                        1 as is_crm_biz
                    FROM "//home/cloud_analytics/import/crm/business_accounts/data"
                ) as t1
                ON t0.billing_account_id = t1.billing_account_id
                WHERE
                    (ba_person_type = 'company'
                    OR is_corporate_card = 1
                    OR is_crm_biz = 1)
                ) as main
                ANY LEFT JOIN(
                    SELECT
                        billing_account_id,
                        MAX(event_time) as last_call_time,
                        argMax(lead_source, event_time) as last_source
                    FROM "//home/cloud_analytics_test/cubes/crm_leads/cube"
                    WHERE
                        event = 'call'
                        AND (billing_account_id != '' OR billing_account_id IS NOT NULL)
                        AND billing_account_id NOT IN (SELECT DISTINCT ba_id FROM "//home/cloud_analytics/export/crm/mql/2019-10-21T10:00:00")
                    GROUP BY
                        billing_account_id
                ) as calls
                ON main.billing_account_id = calls.billing_account_id
            ) as main_call
            ANY LEFT JOIN(
                SELECT
                    billing_account_id,
                    AVG(paid) as paid_avg,
                    stddevPop(paid) as paid_std
                FROM(
                    SELECT
                        t0.billing_account_id,
                        t0.date,
                        multiIf(t1.paid IS NULL, 0 , t1.paid) as paid
                    FROM(
                        SELECT
                            billing_account_id,
                            dates as date
                        FROM(
                            SELECT
                            billing_account_id,
                                arrayMap(x -> addDays(addDays(toDate(NOW()), -14), x) ,range(assumeNotNull(toUInt32(  addDays(toDate(NOW()), 0) -  addDays(toDate(NOW()), -14))    )) ) as dates
                            FROM(
                                SELECT
                                    DISTINCT
                                    billing_account_id
                                FROM "//home/cloud_analytics/cubes/acquisition_cube/cube"
                                WHERE
                                    event = 'day_use'
                                    AND segment IN ('Mass', 'ISV Program')
                                    AND real_consumption > 0
                                    AND toDate(event_time) >= addDays(toDate(NOW()), -14)
                                    AND toDate(event_time) < addDays(toDate(NOW()), 0)
                            )
                        )
                        ARRAY JOIN dates
                    ) as t0
                    ANY LEFT JOIN(
                        SELECT
                            billing_account_id,
                            toDate(event_time) as date,
                            SUM(real_consumption) as paid
                        FROM "//home/cloud_analytics/cubes/acquisition_cube/cube"
                        WHERE
                            event = 'day_use'
                            AND real_consumption_vat > 0
                            AND toDate(event_time) >= addDays(toDate(NOW()), -14)
                            AND toDate(event_time) < addDays(toDate(NOW()), 0)
                        GROUP BY
                            billing_account_id,
                            date
                    ) as t1
                    ON t0.billing_account_id = t1.billing_account_id AND t0.date = t1.date
                )
                GROUP BY
                    billing_account_id
            ) as con
            ON main_call.billing_account_id = con.billing_account_id
        ) AS res
        ANY LEFT JOIN(
            SELECT
                DISTINCT
                passport_uid as puid,
                timezone
            FROM "//home/cloud_analytics/import/iam/cloud_owners_history"
            WHERE
                puid != ''
        ) as tm
        ON res.puid = tm.puid
        ) as result
        ANY LEFT JOIN(
            SELECT
                total_con.*,
                arrayStringConcat(net_mdb.data_bases, ',') as data_bases
            FROM(
                SELECT
                    billing_account_id,
                    SUM(real_consumption) as total_paid,
                    SUM(multiIf(service_name = 'cloud_ai', real_consumption, 0)) as ai_paid,
                    SUM(multiIf(service_name = 'mdb', real_consumption, 0)) as mdb_paid
                FROM "//home/cloud_analytics/cubes/acquisition_cube/cube"
                WHERE
                    event = 'day_use'
                    AND real_consumption_vat > 0
                GROUP BY
                    billing_account_id
            ) as total_con
            ANY LEFT JOIN(
                SELECT
                    billing_account_id,
                    arraySort(arrayDistinct(groupArray(db))) as data_bases
                FROM "//home/cloud_analytics/import/network-logs/db-on-vm/data"
                WHERE
                    billing_account_id IS NOT NULL
                    AND billing_account_id != ''
                GROUP BY
                    billing_account_id
            ) as net_mdb
            ON total_con.billing_account_id = net_mdb.billing_account_id
        ) net_data
        ON result.billing_account_id = net_data.billing_account_id
    )

    '''.format(folder)
    path = "//home/{0}/smb/upsell/billing_accounts".format(folder)

    chyt_execute_query(
        query=query,
        cluster=cluster,
        alias=alias,
        token=token,
        columns = [],
        create_table_dict = {
            'path': path,
            'tables_dir': tables_dir,
            'cluster': cluster_yt
        }
    )

    now = str(datetime.datetime.now()).split('.')[0]

    query = '''
    CREATE TABLE "//home/{0}/export/crm/upsale/{1}" ENGINE=YtTable() AS
    --INSERT INTO "<append=%false>//home/{0}/smb/upsell/billing_accounts"
    SELECT
        billing_account_id,
        email,
        phone,
        first_name,
        last_name,
        client_name,
        timezone,
        multiIf(
            paid_std*100/paid_avg <= 10.0 AND paid_avg > 0 AND ROUND(total_paid) != ROUND(ai_paid), 'upsell',
            toDate(NOW()) - toDate(last_call_time) > 70, 'contact more then 70 days',
            'other'
        ) as lead_source,
        multiIf(
            data_bases != 'have not DB on VM' AND mdb_paid <= 0, concat('Client Use BD on VM: ', data_bases),
            ''
        ) as description
    FROM "//home/{0}/smb/upsell/billing_accounts"
    WHERE
        phone IS NOT NULL
        AND phone != ''
        AND lead_source != 'other'
        AND billing_account_id NOT IN (SELECT DISTINCT billing_account_id FROM "//home/cloud_analytics/smb/upsell_exp/exp_users" WHERE group = 'test')
        AND billing_account_id NOT IN (SELECT DISTINCT billing_account_id FROM concatYtTablesRange("//home/{0}/export/crm/upsale"))
    '''.format(folder, now)
    path = "//home/{0}/export/crm/upsale/{1}".format(folder, now)

    chyt_execute_query(
        query=query,
        cluster=cluster,
        alias=alias,
        token=token,
        columns = [],
        create_table_dict = {
            'path': path,
            'tables_dir': '//home/cloud_analytics/export/crm/upsale',
            'cluster': cluster_yt
        }
    )

if __name__ == '__main__':
    main()
