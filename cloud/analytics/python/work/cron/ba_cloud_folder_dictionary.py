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
    #alias = "*ch_public"
    token = '%s' % (yt_creds['value']['token'])

    mode = 'prod'
    bot = telebot.TeleBot(telebot_creds['value']['token'])
    cluster_yt = clusters.yt.Hahn(
        token = yt_creds['value']['token'],
        pool = yt_creds['value']['pool']
    )
    if mode == 'test':
        paths_dict_temp = paths_dict_test
        folder = 'cloud_analytics_test'
        tables_dir = "//home/{0}/dictionaries/ids".format(folder)
    elif mode == 'prod':
        paths_dict_temp = paths_dict_prod
        folder = 'cloud_analytics'
        tables_dir = "//home/{0}/dictionaries/ids".format(folder)
    paths_dict = paths_dict_temp.copy()

    job = cluster_yt.job()
    folders = get_last_not_empty_table('//home/cloud_analytics/import/iam/cloud_folders/1h', job)
    #clouds = get_last_not_empty_table('//home/cloud_analytics/import/iam/cloud_owners/1h', job)
    ba_ids = get_last_not_empty_table('//home/logfeller/logs/yc-billing-export-clouds/1h', job)

    query = '''
    CREATE TABLE "//home/{2}/dictionaries/ids/ba_cloud_folder_dict" ENGINE=YtTable() AS
    --INSERT INTO "<append=%false>//home/{2}/dictionaries/ids/ba_cloud_folder_dict"
    SELECT
        t0.*,
        t1.billing_account_id,
        t1.epoch,
        toString(t1.ts) as ts
    FROM(
        SELECT
            DISTINCT
            folder_id,
            cloud_id
        FROM "{0}"
    ) as t0
    ALL LEFT JOIN(
        SELECT
            DISTINCT
            id as cloud_id,
            billing_account_id,
            effective_time as epoch,
            addHours(toDateTime(effective_time), -3) as ts
        FROM
            "{1}"
    ) as t1
    ON t0.cloud_id = t1.cloud_id
    '''.format(folders, ba_ids, folder)
    path = "//home/{0}/dictionaries/ids/ba_cloud_folder_dict".format(folder)

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
