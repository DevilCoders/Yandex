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

def main():
    cluster = 'hahn'
    alias = "*cloud_analytics"
    token = '%s' % (yt_creds['value']['token'])

    mode = 'test'
    bot = telebot.TeleBot(telebot_creds['value']['token'])
    cluster_yt = clusters.yt.Hahn(
        token = yt_creds['value']['token'],
        pool = yt_creds['value']['pool']
    )
    if mode == 'test':
        paths_dict_temp = paths_dict_test
        folder = 'cloud_analytics_test'
        tables_dir = "//home/{0}/cooking_cubes/acquisition_cube/sources".format(folder)
    elif mode == 'prod':
        paths_dict_temp = paths_dict_prod
        folder = 'cloud_analytics'
        tables_dir = "//home/{0}/cooking_cubes/acquisition_cube/sources".format(folder)
    paths_dict = paths_dict_temp.copy()

    job = cluster_yt.job()
    for path in['billing_accounts_history_path', 'billing_accounts_path']:
        valid_path = get_last_not_empty_table(paths_dict_temp[path], job)
        paths_dict[path] = valid_path
    fraud_path = get_last_not_empty_table('//home/antifraud/export/cloud/bad_accounts/4h/all', job)
    offers_path = get_last_not_empty_table('//home/logfeller/logs/yc-billing-export-monetary-grant-offers/1h', job)
    grants_path = get_last_not_empty_table('//home/logfeller/logs/yc-billing-export-monetary-grants/1h', job)

    query = '''
    CREATE TABLE "//home/{1}/cooking_cubes/acquisition_cube/sources/promocodes" ENGINE=YtTable() AS
    --INSERT INTO "<append=%false>//home/{1}/cooking_cubes/acquisition_cube/sources/promocodes"
    SELECT
        duration,
        id as promocode_id,
        multiIf(extract(proposed_meta, 'CLOUD[A-Z0-9-]+') = '', 'test', extract(proposed_meta, 'CLOUD[A-Z0-9-]+')) as promocode_reason,
        toFloat64("initial_amount") as promocode_amount,
        toString(toDateTime(expiration_time)) as expiration_time,
        passport_uid as puid,
        proposed_to,
        toString(toDateTime(created_at), 'Etc/UTC') as event_time
    FROM "{0}"
    '''.format(offers_path, folder)
    path = "//home/{0}/cooking_cubes/acquisition_cube/sources/promocodes".format(folder)

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

    query = '''
    CREATE TABLE "//home/{1}/cooking_cubes/acquisition_cube/sources/grants" ENGINE=YtTable() AS
    --INSERT INTO "<append=%false>//home/{1}/cooking_cubes/acquisition_cube/sources/grants"
    SELECT
        id as grant_id,
        billing_account_id,
        source as grant_source,
        toFloat64(initial_amount) as grant_amount,
        toString(toDateTime(created_at), 'Etc/UTC') as grant_creation_time,
        toString(toDateTime(start_time), 'Etc/UTC') as grant_start_time,
        toString(toDateTime(end_time), 'Etc/UTC') as grant_end_time,
        source_id as grant_source_id
    FROM "{0}"
    '''.format(grants_path, folder)
    path = "//home/{0}/cooking_cubes/acquisition_cube/sources/grants".format(folder)

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

    offset = 10
    for sample in range(offset):
        if sample == 0:
            fr_query = 'CREATE TABLE "//home/{0}/cooking_cubes/acquisition_cube/sources/ba_history_" ENGINE = YtTable() AS\n'.format(folder)
        else:
            fr_query = 'INSERT INTO "<append=%true>//home/{0}/cooking_cubes/acquisition_cube/sources/ba_history_"\n'.format(folder)
        query = '''
            SELECT
                multiIf(
                    block_reason != '',lower(block_reason),
                    'Unlocked'
                ) as block_reason,
                multiIf(
                    unblock_reason != '',lower(unblock_reason),
                    ''
                ) as unblock_reason,
                multiIf(
                    is_verified != '',lower(is_verified),
                    ''
                )as is_verified,
                billing_account_id,
                days_ as date,
                updated_at,
                currency,
                payment_cycle_type,
                payment_method_id,
                balance_client_id,
                state,
                type,
                metadata,
                usage_status,
                feature_flags,
                client_id,
                master_account_id,
                iso_eventtime,
                name,
                person_type,
                balance_contract_id,
                payment_type,
                balance,
                billing_threshold
            FROM(
                SELECT
                    t0.*,
                    t1.date_next,
                    arrayMap(x -> toString(addDays(toDate(t0.date), x)) ,range(assumeNotNull(toUInt32(  t1.date_next -  t0.date    ))) ) as days_
                FROM(
                    SELECT
                        billing_account_id,
                        date,
                        groupArray(updated_at)[1] as updated_at,
                        groupArray(currency)[1] as currency,
                        groupArray(payment_cycle_type)[1] as payment_cycle_type,
                        groupArray(payment_method_id)[1] as payment_method_id,
                        groupArray(balance_client_id)[1] as balance_client_id,
                        groupArray(state)[1] as state,
                        groupArray(person_type)[1] as person_type,
                        groupArray(type)[1] as type,
                        groupArray(usage_status)[1] as usage_status,
                        toString(groupArray(feature_flags)[1]) as feature_flags,
                        groupArray(master_account_id)[1] as master_account_id,
                        groupArray(created_at)[1] as created_at,
                        groupArray(balance_contract_id)[1] as balance_contract_id,
                        groupArray(metadata)[1] as metadata,
                        groupArray(payment_type)[1] as payment_type,
                        groupArray(balance)[1] as balance,
                        groupArray(billing_threshold)[1] as billing_threshold,
                        groupArray(block_reason)[1] as block_reason,
                        groupArray(unblock_reason)[1] as unblock_reason,
                        groupArray(client_id)[1] as client_id,
                        groupArray(iso_eventtime)[1] as iso_eventtime,
                        groupArray(name)[1] as name,
                        groupArray(is_verified)[1] as is_verified
                    FROM (
                        SELECT
                            *,
                            toDate(multiIf(billing_account_id = 'dn2mdsrj2sjj6k9pmfnk' AND updated_at = 1565703709, toUInt64(1564655622),updated_at), 'Etc/UTC') as date,
                            replaceRegexpAll(extract(metadata, '"block_reason\": \"[a-z _]+\"'), '("block_reason\": \"|\")', '') as block_reason,
                            replaceRegexpAll(extract(metadata, '"unblock_reason\": \"[a-z _]+\"'), '("unblock_reason\": \"|\")', '') as unblock_reason,
                            replaceRegexpAll(extract(metadata, '"verified\": [a-z]+'), '(verified|:| |\")', '') as is_verified
                        FROM "{0}"
                        WHERE
                            modulo(sipHash64(billing_account_id), {2}) = {3}
                        ORDER BY
                            updated_at DESC
                        )
                    GROUP BY
                        billing_account_id,
                        date
                    ORDER BY
                        billing_account_id,
                        date
                ) as t0
                ANY LEFT JOIN (
                    SELECT
                        billing_account_id,
                        dates as date,
                        dates_ as date_next
                    FROM(
                        SELECT
                            billing_account_id,
                            groupArray(date) as dates,
                            arrayConcat(arraySlice(dates, 2,length(dates)), [addDays(toDate(NOW()), 1)]) as dates_
                        FROM(
                            SELECT
                                DISTINCT
                                billing_account_id,
                                toDate(multiIf(billing_account_id = 'dn2mdsrj2sjj6k9pmfnk' AND updated_at = 1565703709, toUInt64(1564655622),updated_at), 'Etc/UTC') as date
                            FROM "{0}"
                            WHERE
                                modulo(sipHash64(billing_account_id), {2}) = {3}
                            ORDER BY
                                billing_account_id,
                                date
                        )
                        GROUP BY
                            billing_account_id
                    )
                    ARRAY JOIN dates, dates_
                ) as t1
                ON t0.billing_account_id = t1.billing_account_id AND t0.date = t1.date
                ORDER BY
                    billing_account_id,
                    date
            )
            ARRAY JOIN days_
        '''.format(paths_dict['billing_accounts_history_path'], folder, offset, sample)
        query = fr_query + query
        if sample == 0:
            path = "//home/{0}/cooking_cubes/acquisition_cube/sources/ba_history_".format(folder)
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
        else:
            chyt_execute_query(query=query, cluster=cluster, alias=alias, token=token, columns = [])
        print(sample)

    cnx = pymysql.connect(
        user='cloud8',
        password=crm_sql_creds['value']['readonly'],
        host='c-mdb8t5pqa6cptk82ukmc.rw.db.yandex.net',
        port = 3306,
        database='cloud8'
    )

    query = '''
        SELECT
            ba.ba_id as billing_account_id,
            acc_hist.parent_id as account_id,
            CAST(DATE_ADD(date_created, INTERVAL -3 HOUR) AS CHAR) as event_time,
            'changes_sales' as event,
            IFNULL(l1.user_name,'') sales_name,
            acc.name as account_name,
            CASE
                WHEN acc.rating = 'none' OR acc.rating IS NULL THEN 'unknown'
                ELSE acc.rating
            END as potential,
            group_concat(IFNULL(acc_t.user_name,'no_architect')) architects
        FROM accounts_audit as acc_hist
        LEFT JOIN users l1
        ON acc_hist.after_value_string=l1.id AND l1.deleted=0
        LEFT JOIN accounts as acc
        ON acc_hist.parent_id=acc.id
        LEFT JOIN billingaccounts as ba
        ON acc_hist.parent_id=ba.account_id
        LEFT JOIN team_sets
        ON acc.team_set_id = team_sets.id
        LEFT JOIN team_sets_teams
        ON team_sets.id = team_sets_teams.team_set_id
        LEFT JOIN teams
        ON team_sets_teams.team_id = teams.id
        LEFT JOIN users as acc_t
        ON acc_t.id = teams.associated_user_id
        WHERE
            field_name = 'assigned_user_id'
            AND ba.ba_id IS NOT NULL
        GROUP BY
            billing_account_id,
            account_id,
            event_time,
            event,
            sales_name,
            account_name,
            potential
        UNION ALL
        SELECT
            ba.ba_id as billing_account_id,
            acc.id as account_id,
            CAST(DATE_ADD(acc.date_entered, INTERVAL -3 HOUR) AS CHAR) as event_time,
            'changes_sales' as event,
            IFNULL(l1.user_name,'') sales_name,
            acc.name as account_name,
            CASE
                WHEN acc.rating = 'none' OR acc.rating IS NULL THEN 'unknown'
                ELSE acc.rating
            END as potential,
            group_concat(IFNULL(acc_t.user_name,'no_architect')) architects
        FROM accounts as acc
        LEFT JOIN users l1
        ON acc.assigned_user_id=l1.id AND l1.deleted=0
        LEFT JOIN billingaccounts as ba
        ON acc.id=ba.account_id
        LEFT JOIN team_sets
        ON acc.team_set_id = team_sets.id
        LEFT JOIN team_sets_teams
        ON team_sets.id = team_sets_teams.team_set_id
        LEFT JOIN teams
        ON team_sets_teams.team_id = teams.id
        LEFT JOIN users as acc_t
        ON acc_t.id = teams.associated_user_id
        WHERE
            ba.ba_id IS NOT NULL
        GROUP BY
            billing_account_id,
            account_id,
            event_time,
            event,
            sales_name,
            account_name,
            potential
    '''

    account_changes = pd.read_sql_query(query, cnx)
    cnx.close()

    cluster_yt.write(paths_dict['sales'], account_changes)

    schema = {
        "account_id": str,
        "billing_account_id": str,
        "event": str,
        "event_time": str,
        "sales_name": str,
        "account_name":str,
        "architects": str,
        "potential": str
    }
    job = cluster_yt.job()
    job.table(paths_dict['sales']) \
    .project(
        **apply_types_in_project(schema)
    ) \
    .put(paths_dict['sales'] + '_schema', schema = schema)
    job.run()

    query = '''
    CREATE TABLE "//home/{1}/cooking_cubes/acquisition_cube/sources/sales_history_" ENGINE=YtTable() AS
    --INSERT INTO "<append=%false>//home/{1}/cooking_cubes/acquisition_cube/sources/sales_history_"
    SELECT
        billing_account_id,
        days_ as date,
        sales_name,
        architects,
        account_name,
        potential
    FROM(
        SELECT
            t0.*,
            t1.account_name,
            t1.date_next,
            arrayMap(x -> toString(addDays(toDate(t0.date), x)) ,range(assumeNotNull(toUInt32( t1.date_next -  t0.date    ))) ) as days_
        FROM(
            SELECT
                billing_account_id,
                toDate(event_time) as date,
                groupArray(sales_name)[1] as sales_name,
                groupArray(architects)[1] as architects,
                groupArray(potential)[1] as potential
            FROM (SELECT * FROM "//home/{1}/cooking_cubes/acquisition_cube/sources/account_sales_changes_schema" ORDER BY event_time DESC)
            GROUP BY
                billing_account_id,
                date
            ORDER BY
                billing_account_id,
                date
        ) as t0
        ANY LEFT JOIN (
            SELECT
                billing_account_id,
                account_name,
                dates as date,
                dates_ as date_next
            FROM(
                SELECT
                    billing_account_id,
                    account_name,
                    groupArray(date) as dates,
                    arrayConcat(arraySlice(dates, 2,length(dates)), [addDays(toDate(NOW()), 1)]) as dates_
                FROM(
                    SELECT
                        DISTINCT
                        billing_account_id,
                        account_name,
                        dates as date
                    FROM(
                        SELECT
                            billing_account_id,
                            groupArray(toDate(event_time)) as dates,
                            groupArray(account_name)[1] as account_name
                        FROM (SELECT * FROM "//home/{1}/cooking_cubes/acquisition_cube/sources/account_sales_changes_schema" ORDER BY event_time DESC)
                        GROUP BY
                            billing_account_id
                    )
                    ARRAY JOIN dates
                    ORDER BY
                        date
                )
                GROUP BY
                    billing_account_id,
                    account_name
            )
            ARRAY JOIN dates, dates_
        ) as t1
        ON t0.billing_account_id = t1.billing_account_id AND t0.date = t1.date
        ORDER BY
            billing_account_id,
            date
    )
    ARRAY JOIN days_
    '''.format(paths_dict['billing_accounts_history_path'], folder)
    path = "//home/{0}/cooking_cubes/acquisition_cube/sources/sales_history_".format(folder)

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

    query = '''
    CREATE TABLE "//home/{1}/cooking_cubes/acquisition_cube/sources/ba_hist_" ENGINE=YtTable() AS
    --INSERT INTO "<append=%false>//home/{1}/cooking_cubes/acquisition_cube/sources/ba_hist_"
    SELECT
        block_reason,
        unblock_reason,
        is_verified,
        billing_account_id,
        date,
        updated_at,
        currency,
        payment_cycle_type,
        payment_method_id,
        balance_client_id,
        state,
        type,
        metadata,
        usage_status,
        feature_flags,
        client_id,
        master_account_id,
        iso_eventtime,
        name,
        person_type,
        balance_contract_id,
        payment_type,
        balance,
        billing_threshold,
        sales_name,
        is_var,
        architects,
        architect,
        segment,
        account_name,
        potential
    FROM(
        SELECT
            gen.*,
            multiIf(
                account_name IS NULL OR account_name = '', 'unknown', account_name
            ) as account_name
        FROM(
            SELECT
                t0.*,
                multiIf(
                    feature_flags LIKE '%"isv": true%' AND (NOT match(sales_name, '(andreigusev|niktk|ebelobrova)') OR sales_name IS NULL), 'marinapolik',
                    sales_name = '' OR sales_name IS NULL OR lower(sales_name) = 'admin', 'unmanaged',
                    sales_name
                ) as sales_name,
                multiIf(
                    feature_flags LIKE '%"var\": true%', 'var',
                    'not var'
                ) as is_var,
                architects,
                multiIf(
                    architects LIKE '%vetkasov%' AND architects LIKE '%vsgrab%' AND architects LIKE '%nrkk%', 'vetkasov,vsgrab,nrkk',
                    architects LIKE '%vetkasov%' AND architects LIKE '%vsgrab%', 'vetkasov,vsgrab',
                    architects LIKE '%vetkasov%' AND architects LIKE '%nrkk%', 'vetkasov,nrkk',
                    architects LIKE '%vsgrab%' AND architects LIKE '%nrkk%', 'vsgrab,nrkk',
                    architects LIKE '%vetkasov%', 'vetkasov',
                    architects LIKE '%vsgrab%', 'vsgrab',
                    architects LIKE '%nrkk%', 'nrkk',
                    'no_architect'
                ) as architect,
                multiIf(
                    billing_account_id IN ('dn29l2trhqtsgsbcef7v','dn2omk0rbbfem8fp45h7','dn2g9epj20bvoki08l68','dn2dpajtgo1dqijmdbd1','dn26b3csamglqku638an','dn2e8m4uholkpaq4cs0o','dn286bdvlr7tbom4p5bh','dn2c1s8hqdqn3h9099n2','dn2g7qpf57ngcp8so0ah','dn2panp6egst9iidsh5a'), 'Yandex Projects',
                    billing_account_id IN (SELECT DISTINCT billing_account_id FROM "//home/{1}/cooking_cubes/acquisition_cube/sources/grants" WHERE grant_source_id LIKE '%CLOUDBOUNTY%'), 'Yandex Staff',
                    feature_flags LIKE '%"var\": true%' OR (master_account_id != '' AND master_account_id IS NOT NULL), 'VAR',
                    sales_name IN ('golubin', 'datishin', 'sergeykn', 'alexche'), 'Enterprise',
                    sales_name IN ('andreigusev','obelkin') OR billing_account_id IN ('dn2dtdnj2i6beqsbiq4q','dn2iiqtf27l25b8pr0to'), 'Large ISV',
                    sales_name IN ('ebelobrova','niktk'), 'ISV ML',
                    sales_name IN ('glebmarkevich','kalifornia'), 'Medium',
                    feature_flags LIKE '%"isv\": true%' OR sales_name = 'marinapolik', 'ISV Program',
                    'Mass'
                ) as segment,
                multiIf(
                    potential IS NULL OR potential = '', 'unknown', potential
                ) as potential
            FROM(
                SELECT
                    *
                FROM "//home/{1}/cooking_cubes/acquisition_cube/sources/ba_history_"
            ) as t0
            ANY LEFT JOIN (
                SELECT
                    billing_account_id,
                    date,
                    sales_name,
                    architects,
                    potential
                FROM "//home/{1}/cooking_cubes/acquisition_cube/sources/sales_history_"
            ) as t1
            ON t0.billing_account_id = t1.billing_account_id AND t0.date = t1.date
        ) as gen
        ANY LEFT JOIN(
                    SELECT
                    billing_account_id,
                    groupArray(account_name)[1] as account_name
                FROM (SELECT * FROM "//home/{1}/cooking_cubes/acquisition_cube/sources/sales_history_" ORDER BY date DESC)
                GROUP BY
                    billing_account_id
        ) as acc_name
        ON gen.billing_account_id = acc_name.billing_account_id
    )
    '''.format(paths_dict['billing_accounts_history_path'], folder)

    path = "//home/{0}/cooking_cubes/acquisition_cube/sources/ba_hist_".format(folder)

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

    query = '''
    CREATE TABLE "//home/{1}/cooking_cubes/acquisition_cube/sources/ba_hist" ENGINE=YtTable() AS
    --INSERT INTO "<append=%false>//home/{1}/cooking_cubes/acquisition_cube/sources/ba_hist"
    SELECT
        source_1.*,
        grants.grant_sources,
        grants.grant_amounts,
        grants.grant_amount,
        grants.grant_source_ids,
        grants.grant_start_times,
        grants.grant_end_times,
        grants.active_grant_ids,
        grants.active_grants
    FROM(
        SELECT
            source.*,
            multiIf(seg.board_segment IS NULL OR seg.board_segment = '', 'mass', seg.board_segment) as board_segment
        FROM(
            SELECT
                t0.*,
                t1.owner_id,
                multiIf(
                    is_fraud = 1, 1, 0
                ) as is_fraud
            FROM(
                SELECT
                    *
                FROM "//home/{1}/cooking_cubes/acquisition_cube/sources/ba_hist_"
            ) as t0
            ANY LEFT JOIN (
                SELECT
                    t2.*,
                    t3.is_fraud
                FROM(
                    SELECT
                        id as billing_account_id,
                        owner_id
                    FROM "{0}"
                ) as t2
                ANY LEFT JOIN(
                    SELECT
                        DISTINCT
                        toString(passport_uid) as owner_id,
                        multiIf(
                            is_bad = 1 OR recalc_rules IS NOT NULL, 1,0
                        ) as is_fraud
                    FROM "{2}"
                ) as t3
                ON t2.owner_id = t3.owner_id
            ) as t1
            ON t0.billing_account_id = t1.billing_account_id
        ) as source
        ANY LEFT JOIN(
            SELECT
                con.billing_account_id,
                multiIf(
                    is_large = 1 OR con.paid >= 300000, 'large',
                    con.paid >= 10000, 'medium',
                    'mass'
                ) as board_segment
            FROM(
                SELECT
                    billing_account_id,
                    MAX(paid) as paid
                FROM(
                    SELECT
                        billing_account_id,
                        toStartOfMonth(toDate(event_time)) as month,
                        SUM(real_consumption) as paid
                    FROM "//home/{1}/cubes/acquisition_cube/cube"
                    WHERE
                        event = 'day_use'
                    GROUP BY
                        billing_account_id,
                        month
                    ORDER BY
                        billing_account_id,
                        month
                )
                GROUP BY
                    billing_account_id
            ) as con
            ANY LEFT JOIN (
                SELECT
                    DISTINCT
                    billing_account_id,
                    1 as is_large
                FROM(
                    SELECT
                        DISTINCT
                        billing_account_id
                    FROM "//home/cloud_analytics/import/segment/large"
                    UNION ALL
                    SELECT
                        DISTINCT
                        billing_account_id
                    FROM "//home/{1}/cubes/acquisition_cube/cube"
                    WHERE
                        event = 'ba_created'
                        AND sales_name IN ('golubin', 'datishin', 'sergeykn', 'alexche')
                )
            ) as large
            ON con.billing_account_id = large.billing_account_id
        ) as seg
        ON source.billing_account_id = seg.billing_account_id
    ) as source_1
    ANY LEFT JOIN (
        SELECT
            billing_account_id,
            date,
            arrayStringConcat(groupArray(grant_source), ',') as grant_sources,
            arrayStringConcat(groupArray(toString(grant_sum) ), ',') as grant_amounts,
            arraySum(groupArray(grant_sum ) ) as grant_amount,
            arrayStringConcat(groupArray(grant_source_id), ',') as grant_source_ids,
            arrayStringConcat(groupArray(grant_start_time ), ',') as grant_start_times,
            arrayStringConcat(groupArray(grant_end_time ), ',') as grant_end_times,
            arrayStringConcat(groupArray(grant_id), ',') as active_grant_ids,
            length(groupArray(grant_id)) as active_grants
        FROM(
            SELECT
                grant_id,
                billing_account_id,
                grant_source,
                grant_amount as grant_sum,
                grant_source_id,
                grant_start_time,
                grant_end_time,
                days_ as date
            FROM(
                SELECT
                    grant_id,
                    billing_account_id,
                    grant_source,
                    grant_amount,
                    grant_source_id,
                    grant_start_time,
                    grant_end_time,
                    arrayMap(x -> toString(addDays(toDate(grant_start_time), x)) ,range(assumeNotNull(toUInt32(  toDate(grant_end_time) -  toDate(grant_start_time)    ))) ) as days_
                FROM "//home/{1}/cooking_cubes/acquisition_cube/sources/grants"
                WHERE
                    grant_start_time <= toString(NOW())
            )
            ARRAY JOIN days_
            ORDER BY
                billing_account_id,
                grant_id,
                date
        )
        GROUP BY
            billing_account_id,
            date
    ) as grants
    ON grants.billing_account_id = source_1.billing_account_id AND grants.date = source_1.date
    '''.format(paths_dict['billing_accounts_path'], folder, fraud_path)
    path = "//home/{0}/cooking_cubes/acquisition_cube/sources/ba_hist".format(folder)

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
if __name__ == '__main__':
    main()
