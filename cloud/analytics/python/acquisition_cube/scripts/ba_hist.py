#!/usr/bin/env python2
# -*- coding: utf-8 -*-
import pandas as pd, datetime, numpy as np, json, requests, os, sys, pymysql,time
from requests.exceptions import HTTPError
import time
import click

from nile.api.v1 import (
    clusters,
    aggregators as na,
    extractors as ne,
    filters as nf,
    Record
)


paths_dict_prod = {
    'billing_accounts_history_path': '//home/cloud/billing/exported-billing-tables/billing_accounts_history_prod',
    'billing_accounts_path': '//home/cloud/billing/exported-billing-tables/billing_accounts_prod',
    'billing_records_path': '//home/cloud/billing/exported-billing-tables/billing_records_realtime_prod',
    'billing_grants': '//home/cloud/billing/exported-billing-tables/monetary_grants_prod',
    'sku_path': '//home/cloud/billing/exported-billing-tables/skus_prod',
    'cloud_owner_path': '//home/cloud_analytics/import/iam/cloud_owners_history',
    'transactions_path': '//home/cloud/billing/exported-billing-tables/transactions_prod',
    'offers_path': '//home/cloud_analytics/import/billing2/offers/2018-10-31',
    'visits_path' : '//home/cloud_analytics/cooking_cubes/acquisition_cube/sources/visits',
    'client_segments': '//home/cloud_analytics/import/wiki/clients_segments',
    'cube_tmp':'//home/cloud_analytics/cubes/acquisition_cube',
    'funnel_cube': '//home/cloud_analytics/cubes/acquisition_cube',
    'service_dict_path': '//home/cloud/billing/exported-billing-tables/services_prod',
    'balance': '//home/cloud_analytics/import/balance/balance_persons',
    'calls': '//home/cloud_analytics/cooking_cubes/acquisition_cube/sources/calls',
    'open_mail': '//home/cloud_analytics/cooking_cubes/acquisition_cube/sources/open_mail',
    'click_email': '//home/cloud_analytics/cooking_cubes/acquisition_cube/sources/click_email',
    'ba_hist': '//home/cloud_analytics/cooking_cubes/acquisition_cube/sources/ba_hist',
    'grants': '//home/cloud_analytics/cooking_cubes/acquisition_cube/sources/grants_hist',
    'sales': '//home/cloud_analytics/cooking_cubes/acquisition_cube/sources/account_sales_changes'
}

paths_dict_test = {
    'billing_accounts_history_path': '//home/cloud/billing/exported-billing-tables/billing_accounts_history_prod',
    'billing_accounts_path': '//home/cloud/billing/exported-billing-tables/billing_accounts_prod',
    'billing_records_path': '//home/cloud/billing/exported-billing-tables/billing_records_realtime_prod',
    'billing_grants': '//home/cloud/billing/exported-billing-tables/monetary_grants_prod',
    'sku_path': '//home/cloud/billing/exported-billing-tables/skus_prod',
    'cloud_owner_path': '//home/cloud_analytics/import/iam/cloud_owners_history',
    'transactions_path': '//home/cloud/billing/exported-billing-tables/transactions_prod',
    'offers_path': '//home/cloud_analytics/import/billing2/offers/2018-10-31',
    'visits_path' : '//home/cloud_analytics/test/cooking_cubes/acquisition_cube/sources/visits',
    'client_segments': '//home/cloud_analytics/import/wiki/clients_segments',
    'cube_tmp':'//home/cloud_analytics/test/cubes/acquisition_cube',
    'funnel_cube': '//home/cloud_analytics/test/cubes/acquisition_cube',
    'service_dict_path': '//home/cloud/billing/exported-billing-tables/services_prod',
    'balance': '//home/cloud_analytics/import/balance/balance_persons',
    'calls': '//home/cloud_analytics/test/cooking_cubes/acquisition_cube/sources/calls',
    'open_mail': '//home/cloud_analytics/test/cooking_cubes/acquisition_cube/sources/open_mail',
    'click_email': '//home/cloud_analytics/test/cooking_cubes/acquisition_cube/sources/click_email',
    'ba_hist': '//home/cloud_analytics/test/cooking_cubes/acquisition_cube/sources/ba_hist',
    'grants': '//home/cloud_analytics/test/cooking_cubes/acquisition_cube/sources/grants_hist',
    'sales': '//home/cloud_analytics/test/cooking_cubes/acquisition_cube/sources/account_sales_changes'
}

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
    create_table_dict = {}
):
    print("seeping for 20 sec to avoid chyt lag")
    time.sleep(20)
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

@click.command()
@click.option('--prod', is_flag=True)
@click.option('--ch_token')
@click.option('--tracker_token')
def main(ch_token, tracker_token, prod):

    yt_creds = {
        'value': {
            'pool': 'cloud_analytics_pool',
            'token': ch_token
        }
    }

    crm_sql_creds = {
        'value': {
            'readonly': tracker_token
        }
    }

    cluster = 'hahn'
    alias = "*cloud_analytics"
    #alias = "*ch_public"
    token = '%s' % (yt_creds['value']['token'])

    paths_dict_temp = None
    folder = None
    tables_dir = None
    cluster = 'hahn'
    alias = "*cloud_analytics"
    #alias = "*ch_public"
    token = '%s' % (yt_creds['value']['token'])

    cluster_yt = clusters.yt.Hahn(
        token = yt_creds['value']['token'],
        pool = yt_creds['value']['pool']
    )
    if not prod:
        paths_dict_temp = paths_dict_test
        folder = 'cloud_analytics/test'
        tables_dir = "//home/{0}/cooking_cubes/acquisition_cube/sources".format(folder)
    elif prod:
        paths_dict_temp = paths_dict_prod
        folder = 'cloud_analytics'
        tables_dir = "//home/{0}/cooking_cubes/acquisition_cube/sources".format(folder)
    paths_dict = paths_dict_temp.copy()

    job = cluster_yt.job()
    #for path in['billing_accounts_history_path', 'billing_accounts_path']:
        #valid_path = get_last_not_empty_table(paths_dict_temp[path], job)
        #paths_dict[path] = valid_path
    fraud_path = get_last_not_empty_table('//home/antifraud/export/cloud/bad_accounts/1h', job)
    offers_path = '//home/cloud/billing/exported-billing-tables/monetary_grant_offers_prod'
    grants_path = '//home/cloud/billing/exported-billing-tables/monetary_grants_prod'

    query = '''
    CREATE TABLE "//home/{1}/cooking_cubes/acquisition_cube/sources/promocodes" ENGINE = YtTable('{{primary_medium=ssd_blobs}}') AS
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
    CREATE TABLE "//home/{1}/cooking_cubes/acquisition_cube/sources/grants" ENGINE = YtTable('{{primary_medium=ssd_blobs}}') AS
    --INSERT INTO "<append=%false>//home/{1}/cooking_cubes/acquisition_cube/sources/grants"
    SELECT
        id as grant_id,
        billing_account_id,
        source as grant_source,
        toFloat64(initial_amount) as grant_amount,
        toString(toDateTime(created_at), 'Etc/UTC') as grant_creation_time,
        toString(toDateTime(start_time), 'Etc/UTC') as grant_start_time,
        toString(toDateTime(end_time), 'Etc/UTC') as grant_end_time,
        replaceAll(source_id, ',', '') as grant_source_id
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
            fr_query = '''CREATE TABLE "//home/{0}/cooking_cubes/acquisition_cube/sources/ba_history_" ENGINE = YtTable('{{primary_medium=ssd_blobs}}') AS\n'''.format(folder)
        else:
            fr_query = 'INSERT INTO "<append=%true>//home/{0}/cooking_cubes/acquisition_cube/sources/ba_history_"\n'.format(folder)
        query = '''
            SELECT
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
                        groupArray(multiIf(billing_account_id = 'dn23tf22je8ehdv1f4d5', 'service', usage_status))[1] as usage_status,
                        toString(groupArray(feature_flags)[1]) as feature_flags,
                        groupArray(master_account_id)[1] as master_account_id,
                        groupArray(created_at)[1] as created_at,
                        groupArray(balance_contract_id)[1] as balance_contract_id,
                        groupArray(metadata)[1] as metadata,
                        groupArray(payment_type)[1] as payment_type,
                        groupArray(balance)[1] as balance,
                        groupArray(billing_threshold)[1] as billing_threshold,
                        groupArray(client_id)[1] as client_id,
                        groupArray(name)[1] as name
                    FROM (
                        SELECT
                            *,
                            toDate(multiIf(billing_account_id = 'dn2mdsrj2sjj6k9pmfnk' AND updated_at = 1565703709, toUInt64(1564655622),updated_at), 'Etc/UTC') as date
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
        host='c-mdb8t5pqa6cptk82ukmc.ro.db.yandex.net',
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
    CREATE TABLE "//home/{1}/cooking_cubes/acquisition_cube/sources/sales_history_" ENGINE = YtTable('{{primary_medium=ssd_blobs}}') AS
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
    CREATE TABLE "//home/{1}/cooking_cubes/acquisition_cube/sources/ba_hist_" ENGINE = YtTable('{{primary_medium=ssd_blobs}}') AS
    --INSERT INTO "<append=%false>//home/{1}/cooking_cubes/acquisition_cube/sources/ba_hist_"
    SELECT
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
        multiIf(billing_account_id IN (SELECT DISTINCT id FROM "//home/cloud/billing/exported-billing-tables/billing_accounts_prod" WHERE usage_status = 'service'),'service',usage_status) as usage_status,
        feature_flags,
        client_id,
        master_account_id,
        name,
        person_type,
        balance_contract_id,
        payment_type,
        balance,
        billing_threshold,
        sales_name,
        is_var,
        is_isv,
        architects,
        architect,
        segment,
        account_name,
        potential,
        is_iot
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
                    sales_name = '' OR sales_name IS NULL OR lower(sales_name) = 'admin', 'unmanaged',
                    sales_name
                ) as sales_name,
                multiIf(
                    feature_flags LIKE '%"var\": true%', 'var',
                    master_account_id != '' AND master_account_id IS NOT NULL, 'var',
                    'not var'
                ) as is_var,
                multiIf(
                    feature_flags LIKE '%"isv\": true%', 1,
                    0
                ) as is_isv,
                architects,
                multiIf(
                    architects LIKE '%vetkasov%' AND architects LIKE '%vsgrab%' AND architects LIKE '%nrkk%', 'vetkasov,vsgrab,nrkk',
                    architects LIKE '%vetkasov%' AND architects LIKE '%vsgrab%', 'vetkasov,vsgrab',
                    architects LIKE '%vetkasov%' AND architects LIKE '%nrkk%', 'vetkasov,nrkk',
                    architects LIKE '%vsgrab%' AND architects LIKE '%nrkk%', 'vsgrab,nrkk',
                    architects LIKE '%vetkasov%', 'vetkasov',
                    architects LIKE '%vsgrab%', 'vsgrab',
                    architects LIKE '%nrkk%', 'nrkk',
                    architects LIKE '%savemech%', 'savemech',
                    architects LIKE '%vachel%', 'vachel',
                    architects LIKE '%alex-vlasov%', 'alex-vlasov',
                    architects LIKE '%im-i%', 'im-i',
                    architects LIKE '%makhlu%', 'makhlu',
                    'no_architect'
                ) as architect,
                multiIf(
                    architects LIKE '%flekon%', 1,
                    0
                ) as is_iot,
                multiIf(
                    sales_name IN ('golubin', 'datishin', 'sergeykn', 'alexche', 'amashika', 'kevdok', 'bogachenko', 'lexponomareva', 'sorokser', 'rodionovdmitr', 'mikhaylenko', 'anton-zverev', 'urazikova', 'kirpinskiy', 'mkulabukhova'), 'Enterprise',
                    sales_name IN ('andreigusev','obelkin', 'dmkonovalov', 'pavelio', 'ebelobrova','niktk', 'maxtsygankov', 'bayazitov', 'adaskal', 'nabiullin', 'k-den') OR billing_account_id IN ('dn2dtdnj2i6beqsbiq4q','dn2iiqtf27l25b8pr0to'), 'Large ISV',
                    --sales_name IN ('ebelobrova','niktk', 'maxtsygankov'), 'ISV ML',
                    sales_name IN ('glebmarkevich','kalifornia', 'dkharlamov', 'andreieremin', 'suchkovandrey', 'esenat', 'galst2k', 'gadirov', 'askryuchkov', 'yulia-mak', 'dmayudin'), 'Medium',
                    billing_account_id IN ('dn2v0gan5jc5pe8tco3o','dn29l2trhqtsgsbcef7v','dn2omk0rbbfem8fp45h7','dn2g9epj20bvoki08l68','dn2dpajtgo1dqijmdbd1','dn26b3csamglqku638an','dn2e8m4uholkpaq4cs0o','dn286bdvlr7tbom4p5bh','dn2c1s8hqdqn3h9099n2','dn2g7qpf57ngcp8so0ah','dn2panp6egst9iidsh5a', 'dn2v0iil8aamjg0hkt3s'), 'Yandex Projects',
                    billing_account_id IN (SELECT DISTINCT billing_account_id FROM "//home/{1}/cooking_cubes/acquisition_cube/sources/grants" WHERE grant_source_id LIKE '%CLOUDBOUNTY%') OR billing_account_id = 'dn2qgvubbbri09uodcol', 'Yandex Staff',
                    master_account_id != '' AND master_account_id IS NOT NULL, 'VAR',
                    feature_flags LIKE '%"isv\": true%', 'ISV Program',
                    feature_flags LIKE '%"var\": true%', 'VAR',
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
    CREATE TABLE "//home/{0}/cooking_cubes/acquisition_cube/sources/grants_history" ENGINE = YtTable('{{primary_medium=ssd_blobs}}') ORDER BY (billing_account_id, date, grant_id) AS
        SELECT
            billing_account_id,
            days_ as date,
            grant_id,
            grant_source,
            grant_amount as grant_sum,
            grant_source_id,
            grant_start_time,
            grant_end_time
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
            FROM "//home/{0}/cooking_cubes/acquisition_cube/sources/grants"
            WHERE
                grant_start_time <= toString(NOW()) AND grant_end_time >= grant_start_time
        )
        ARRAY JOIN days_
        ORDER BY billing_account_id, date, grant_id
    '''.format(folder)
    path = "//home/{0}/cooking_cubes/acquisition_cube/sources/grants_history".format(folder)

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
    CREATE TABLE "//home/{1}/cooking_cubes/acquisition_cube/sources/ba_hist" ENGINE = YtTable('{{primary_medium=ssd_blobs}}') AS
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
                t1.owner_id as owner_id,
                t1.is_fraud as is_fraud,
                t1.block_comment_curr as block_comment,
                ifNull(lower(t1.is_verified_curr), '')   as is_verified,
                multiIf(
                    block_reason_curr != '',lower(block_reason_curr),
                    'Unlocked'
                ) as block_reason,
                ifNull(lower(t1.unblock_reason_curr), '') as unblock_reason


            FROM(
                SELECT
                    *
                FROM "//home/{1}/cooking_cubes/acquisition_cube/sources/ba_hist_"
            ) as t0
            ANY LEFT JOIN (
                SELECT
                    id as billing_account_id,
                    owner_id,
                    replaceRegexpAll(extract(metadata, '"block_reason\": \"[a-z _]+\"'), '("block_reason\": \"|\")', '') as block_reason_curr,
                    replaceRegexpAll(extract(metadata, '"unblock_reason\": \"[a-z _]+\"'), '("unblock_reason\": \"|\")', '') as unblock_reason_curr,
                    visitParamExtractString(concat('{{"abc":"', replaceRegexpAll(extract(metadata, '"block_comment\": \"[a-z _ 0-9 \\\\\\\\ ]+\"'), '("block_comment\": \"|\")', ''), '"}}'), 'abc') as block_comment_curr,
                    -- It is terrible, but YPath cant't parse it.  Also since it is binnary cyrillyc symbols are represented
                    -- by \u0(smth) and ClickHouse does't have normal function to decode it.
                    -- Anyways it needs to be rewritten.
                    replaceRegexpAll(extract(metadata, '"verified\": [a-z]+'), '(verified|:| |\")', '') as is_verified_curr,

                    multiIf(
                        ifNull(is_verified_curr, 'false') = 'true', 0,
                        billing_account_id IN (SELECT DISTINCT billing_account_id FROM "//home/antifraud/export/cloud/bad_reg/accounts_diff"), 0,
                        ((lower(block_comment_curr) like '%fraud%')
                                    OR (lower(block_comment_curr) like '%фрод%')
                                    )
                                    and ((unblock_reason_curr = '')
                                        or isNull(unblock_reason_curr)), 1,
                        0
                    ) as is_fraud
                FROM "{0}"

            ) as t1
            ON t0.billing_account_id = t1.billing_account_id
        ) as source
        ANY LEFT JOIN(
            SELECT
                con.billing_account_id,
                con.date,
                multiIf(
                    is_large = 1 OR con.is_cum_large = 1, 'large',
                    con.is_cum_medium = 1 AND con.is_cum_large = 0, 'medium',
                    'mass'
                ) as board_segment
            FROM(
                SELECT
                    billing_account_id,
                    month,
                    first_month_day,
                    last_month_day,
                    paid,
                    is_cum_large,
                    is_cum_medium,
                    days_ as date
                FROM(
                    SELECT
                        billing_account_id,
                        months as month,
                        first_month_days as first_month_day,
                        last_month_days as last_month_day,
                        paids as paid,
                        multiIf(is_cum_larges > 0, 1, 0) as is_cum_large,
                        multiIf(is_cum_mediums > 0, 1, 0) as is_cum_medium,
                        arrayMap(x -> toString(addDays(toDate(first_month_day), x)) ,range(assumeNotNull(toUInt32(  last_month_day -  first_month_day    ) + 1)) ) as days_
                    FROM(
                        SELECT
                            billing_account_id,
                            groupArray(month) as months,
                            groupArray(first_month_day) as first_month_days,
                            groupArray(last_month_day) as last_month_days,
                            groupArray(paid) as paids,
                            groupArray(is_large) as is_larges,
                            groupArray(is_medium) as is_mediums,
                            arrayCumSum(is_larges) as is_cum_larges,
                            arrayCumSum(is_mediums) as is_cum_mediums
                        FROM(
                            SELECT
                                billing_account_id,
                                month,
                                MIN(event_time) as first_month_day,
                                MAX(event_time) as last_month_day,
                                SUM(real_consumption) as paid,
                                multiIf(paid > 300000, 1, 0) as is_large,
                                multiIf(paid > 10000, 1, 0) as is_medium
                            FROM(
                                SELECT
                                    billing_account_id,
                                    toDate(event_time) as event_time,
                                    toStartOfMonth(toDate(event_time)) as month,
                                    real_consumption
                                FROM "//home/cloud_analytics/cubes/acquisition_cube/cube"
                                WHERE
                                    event = 'day_use'
                            )
                            GROUP BY
                                billing_account_id,
                                month
                            ORDER BY
                                billing_account_id,
                                month
                        )
                        GROUP BY
                            billing_account_id
                    )
                    ARRAY JOIN months,first_month_days,last_month_days,paids,is_cum_larges,is_cum_mediums
                )
                ARRAY JOIN days_
                ORDER BY
                    date
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
                    FROM "//home/cloud_analytics/cubes/acquisition_cube/cube"
                    WHERE
                        event = 'ba_created'
                        AND sales_name IN ('golubin', 'datishin', 'sergeykn', 'alexche', 'amashika')
                )
            ) as large
            ON con.billing_account_id = large.billing_account_id
                ) as seg
        ON source.billing_account_id = seg.billing_account_id AND source.date = seg.date
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
        FROM "//home/{1}/cooking_cubes/acquisition_cube/sources/grants_history"
		WHERE date < '2022-01-01'
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

    with open('output.json', 'w') as f:
        json.dump({"table_path": path
        }, f)
if __name__ == '__main__':
    main()
