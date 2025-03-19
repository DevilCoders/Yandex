#!/usr/bin/env python2
# -*- coding: utf-8 -*-
import pandas as pd, datetime, ast, os,sys, pymysql, time
module_path = os.path.abspath(os.path.join('/home/ktereshin/yandex/arcadia/cloud/analytics/python/work'))
if module_path not in sys.path:
    sys.path.append(module_path)
module_path = os.path.abspath(os.path.join('/home/ktereshin/yandex/cron'))
if module_path not in sys.path:
    sys.path.append(module_path)
from data_loader import clickhouse
from yt.transfer_manager.client import TransferManager
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
    stat_creds,
    telebot_creds
)

def exucute_query(query, hosts, cloud_clickhouse_param_dict_):
    result = {}
    for host in hosts.split(','):
        try:
            print('start on host: %s' % (host))
            cloud_clickhouse_param_dict_['host'] = host
            cloud_clickhouse_param_dict_['query'] = query
            result[host] = clickhouse.get_clickhouse_data(**cloud_clickhouse_param_dict_)
            print('============================\n\n')
        except:
            break
    return result

def main():
    cluster = clusters.yt.Hahn(
        token = yt_creds['value']['token'],
        pool = yt_creds['value']['pool']
    )
    metrika_clickhouse_param_dict['user'] = metrika_creds['value']['login']
    metrika_clickhouse_param_dict['password'] = metrika_creds['value']['pass']

    cloud_clickhouse_param_dict['user'] = yc_ch_creds['value']['login']
    cloud_clickhouse_param_dict['password'] = yc_ch_creds['value']['pass']


    hosts = 'https://sas-tt9078df91ipro7e.db.yandex.net:8443/,https://vla-2z4ktcci90kq2bu2.db.yandex.net:8443/'

    query = '''
    SELECT
        CONCAT(database, '.', name) as table
    FROM
        system.tables
    WHERE
        match(name, '_[0-9]{10,20}$')
    '''
    res = exucute_query(query, hosts, cloud_clickhouse_param_dict)

    table_lists = []
    for host in hosts.split(','):
        if isinstance(res[host], pd.core.frame.DataFrame):
            table_lists += list(res[host][0])
    table_lists = set(table_lists)

    if len(table_lists)>0:
        for table in table_lists:
            diff = (datetime.datetime.now() - datetime.datetime(1970,1,1)).total_seconds() - (int(table.split('_')[-1]) + 10800)
            if diff > 18000:
                print(table)
                query = 'DROP TABLE IF EXISTS {0}'.format(table)
                exucute_query(query, hosts, cloud_clickhouse_param_dict)
if __name__ == '__main__':
    main()
