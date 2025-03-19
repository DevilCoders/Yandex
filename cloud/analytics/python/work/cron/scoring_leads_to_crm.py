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
    cluster_yt = clusters.yt.Hahn(
        token = yt_creds['value']['token'],
        pool = yt_creds['value']['pool']
    )
    job = cluster_yt.job()
    passport = get_last_not_empty_table('//home/cloud_analytics/import/iam/cloud_owners/1h', job)
    table_path = "//home/cloud_analytics/scoring/mql_testing_crm/{0}".format('T'.join(str(datetime.datetime.now()).split('.')[0].split(' ')))

    query = '''
    CREATE TABLE "{0}" ENGINE = YtTable() AS
    SELECT
        multiIf(
            ba_id IS NULL OR ba_id = '', 'unknown',
            ba_id
        ) as ba_id,
        multiIf(
            email IS NULL OR email = '', 'unknown@yandex.ru',
            email
        ) as email,
        multiIf(
            phone IS NULL OR phone = '', '+78000000000',
            phone
        ) as phone,
        multiIf(
            first_name IS NULL OR first_name = '', 'unknown',
            first_name
        ) as first_name,
        multiIf(
            last_name IS NULL OR last_name = '', 'unknown',
            last_name
        ) as last_name,
        multiIf(
            dwh_score IS NULL OR dwh_score = '', 'unknown',
            dwh_score
        ) as dwh_score,
        multiIf(
            t5.timezone IS NULL OR t5.timezone = '', 'unknown',
            t5.timezone
        ) as timezone,
        'Scoring Leads' as campaign_name,
        multiIf(
            client_name IS NULL OR client_name = '', 'unknown',
            client_name
        ) as client_name,
        'Lead Score' as score_type_id,
        5 as score_points,
        multiIf(
            ba_person_type = 'company' OR is_corporate_card = 1, 'Client is Company',
            'Client is Individual'
        ) as  lead_source_description
    FROM (
        SELECT
            billing_account_id as ba_id,
            puid,
            email,
            phone,
            first_name,
            last_name,
            toString(score) as dwh_score,
            ba_state,
            block_reason,
            ba_person_type,
            client_name,
            score,
            is_corporate_card
        FROM(
            SELECT
                *,
                addDays(toDate(first_trial_consumption_date), 15) as date
            FROM "//home/cloud_analytics/scoring/leads/leads"
            WHERE
                addDays(toDate(first_trial_consumption_date), 15) = toDate(NOW())
        ) as t2
        ANY LEFT JOIN (
            SELECT
                bai.*,
                multiIf(cb.is_yandex_grant IS NULL, 0, cb.is_yandex_grant) as is_yandex_grant
            FROM(
                SELECT
                    billing_account_id,
                    user_settings_email as email,
                    phone,
                    ba_state,
                    block_reason,
                    first_first_paid_consumption_datetime,
                    first_name,
                    last_name,
                    ba_person_type,
                    multiIf(
                        account_name IS NOT NULL AND account_name != 'unknown',account_name,
                        balance_name IS NOT NULL AND balance_name != 'unknown',balance_name,
                        ba_name IS NOT NULL AND ba_name != 'unknown',ba_name,
                        CONCAT(first_name, ' ', last_name)
                    ) as client_name,
                    segment,
                    is_corporate_card
                FROM "//home/cloud_analytics/cubes/acquisition_cube/cube"
                WHERE
                    event IN ('ba_created')
            ) bai
            ANY LEFT JOIN(
                SELECT
                    DISTINCT
                    billing_account_id,
                    1 as is_yandex_grant
                FROM "//home/cloud_analytics/cubes/acquisition_cube/cube"
                WHERE
                    grant_source_ids LIKE '%CLOUDBOUNTY%'
            ) as cb
            ON bai.billing_account_id = cb.billing_account_id
        ) as t3
        ON t2.billing_account_id = t3.billing_account_id
        WHERE
            (score > 0.2 OR is_corporate_card = 1 OR ba_person_type = 'company' )
            AND (block_reason NOT IN ('manual', 'mining') OR block_reason IS NULL)
            AND segment NOT IN ('ISV Program', 'VAR', 'Large ISV', 'ISV ML', 'Medium', 'Enterprise')
            AND is_yandex_grant = 0
    ) as t4
    ANY LEFT JOIN (
        SELECT
            DISTINCT
            passport_uid as puid,
            timezone
        FROM "{1}"
        WHERE
            puid != ''
    ) as t5
    ON t4.puid = t5.puid
    '''.format(table_path, passport)

    chyt_execute_query(
        query=query,
        cluster=cluster,
        alias=alias,
        token=token,
        columns = [],
        create_table_dict = {
            'path': table_path,
            'tables_dir': '//home/cloud_analytics/scoring/mql_testing_crm',
            'cluster': cluster_yt
        }
    )
if __name__ == '__main__':
    main()
