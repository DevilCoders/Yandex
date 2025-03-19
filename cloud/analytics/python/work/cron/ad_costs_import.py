#!/usr/bin/env python2
# -*- coding: utf-8 -*-
import pandas as pd, datetime, ast, numpy as np, telebot, json, requests, os, sys, pymysql
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

def get_service(name_):
    return str(name_).split('.')[0]


def convert_metric_to_float(num):
    try:
        return float(num)
    except:
        return 0.0


def get_payment_type(context):
    try:
        return ast.literal_eval(context)['payment_type']
    except:
        return None

def get_reason(metadata_):
    if metadata_ not in ['', None]:
        metadata =  json.loads(metadata_)
    else:
        metadata = None
    if metadata:
        if 'block_reason' in metadata:
            return metadata['block_reason']
        if 'suspend_reason' in metadata:
            return metadata['suspend_reason']
        #if 'fraud_detected_by' in metadata:
            #if isinstance(metadata['fraud_detected_by'], list):
                #return metadata['fraud_detected_by'][0]
            #else:
                #return metadata['fraud_detected_by'].replace('[', '').replace(']', '').replace("u'", '').replace("'", '')
    return 'unknown'

def get_datetime_from_epoch(epoch):
    try:
        return str(datetime.datetime.fromtimestamp(int(epoch)))
    except:
        return None


def convert_epoch_to_end_day(epoch):
    try:
        return str(datetime.datetime.fromtimestamp(int(epoch)).replace(hour=23).replace(minute=59).replace(second=59))
    except:
        return None
def date_range_by_days(start_str, end_str):
    start = datetime.datetime.strptime(start_str, '%Y-%m-%d')
    end = datetime.datetime.strptime(end_str, '%Y-%m-%d')
    delta = int((end - start).days) + 1
    date_list = []
    for i in range(delta):
        date_list.append( str((start + datetime.timedelta(days = i)).date()) )
    return date_list

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

def chyt_execute_query(query, cluster, alias, token, columns):
    i = 0
    while True:
        try:
            if 'create table' in query.lower() or 'insert into' in query.lower():
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
            if i > 10:
                print('Break Excecution')
                break

def main():
    cluster = 'hahn'
    alias = "*cloud_analytics"
    token = '%s' % (yt_creds['value']['token'])

    mode = 'prod'
    cluster_yt = clusters.yt.Hahn(
        token = yt_creds['value']['token'],
        pool = yt_creds['value']['pool']
    )
    if mode == 'test':
        folder = 'cloud_analytics_test'
    elif mode == 'prod':
        folder = 'cloud_analytics'

    job = cluster_yt.job()
    all_tables = [table for table in sorted(get_table_list('//home/marketing-data/money_log/v2', job)[1:-1].split(',')) if 'dataset' not in table ]


    query = '''
    SELECT
        MAX(date)
    FROM "//home/{0}/import/marketing/ad_costs/ad_costs"
    WHERE
        date NOT LIKE '%test_%'
    '''.format(folder)
    last_table = chyt_execute_query(query=query, cluster=cluster, alias=alias, token=token, columns = ['date'])['date'].iloc[0]


    new_tables = []
    for path in all_tables:
        if path.split('/')[-1] > last_table:
            new_tables.append(path)

    if new_tables:
        res = pd.DataFrame()
        for path in new_tables:
            if path.split('/')[-1] >= '2019-01-15':
                query = '''
                SELECT
                    medium,
                    cost,
                    campaign,
                    cost_usd,
                    source,
                    client,
                    goals,
                    impressions,
                    clicks,
                    '{1}' as date
                FROM "{0}"
                WHERE
                    client = 'Яндекс Облако'
                    AND cost > 0
                '''.format(path, path.split('/')[-1])
                data = chyt_execute_query(query=query, cluster=cluster, alias=alias, token=token, columns = ['medium','cost','campaign','cost_usd','source','client','goals','impressions','clicks','date'])
                if data.shape[0] > 0:
                    res = pd.concat(
                        [
                            res,
                            data
                        ]
                    )
        cluster_yt.write('//home/{0}/import/marketing/ad_costs/ad_costs_'.format(folder), res, append=True)
        schema = {
            "campaign": str,
            "clicks": int,
            "client": str,
            "cost": float,
            "cost_usd": float,
            "date": str,
            "goals": int,
            "impressions": int,
            "medium": str,
            "source": str
        }
        job = cluster_yt.job()
        job.table('//home/{0}/import/marketing/ad_costs/ad_costs_'.format(folder)) \
        .project(
            ne.all(),
            date = ne.custom(lambda x: x if 'test_' not in x else str(x).split('_')[1], 'date')
        ) \
        .project(**apply_types_in_project(schema)) \
        .put('//home/{0}/import/marketing/ad_costs/ad_costs'.format(folder), schema = schema)
        job.run()

if __name__ == '__main__':
    main()
