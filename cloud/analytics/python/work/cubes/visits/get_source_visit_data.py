#!/usr/bin/env python2
# -*- coding: utf-8 -*-
import os, sys, pandas as pd, datetime, telebot
module_path = os.path.abspath(os.path.join('/home/ktereshin/yandex/arcadia/cloud/analytics/python/work'))
if module_path not in sys.path:
    sys.path.append(module_path)
from data_loader import clickhouse
from global_variables import (
    metrika_clickhouse_param_dict,
    cloud_clickhouse_param_dict
)
from init_variables import queries,queries_append_mode
from vault_client import instances

from nile.api.v1 import (
    clusters,
    aggregators as na,
    extractors as ne,
    filters as nf,
    Record
)


def main():
    date = str(datetime.date.today()-datetime.timedelta(days = 1))
    client = instances.Production()
    yt_creds = client.get_version('ver-01d33pgv8pzc7t99s3egm24x47')
    metrika_creds = client.get_version('ver-01d2z36msatt9mp9pcfptezksp')
    yc_ch_creds = client.get_version('ver-01d2z39xj02xw7gqvv9wq757ne')

    paths_dict_test = {
        'visits_event':'//home/cloud_analytics_test/cooking_cubes/acquisition_cube/sources/visits'
    }
    paths_dict_prod = {
        'visits_event':'//home/cloud_analytics/cooking_cubes/acquisition_cube/sources/visits'
    }
    mode = 'test'
    if mode == 'test':
        paths_dict = paths_dict_test
    elif mode == 'prod':
        paths_dict = paths_dict_prod


    cluster = clusters.yt.Hahn(
        token = yt_creds['value']['token'],
        pool = yt_creds['value']['pool']
    )
    job = cluster.job()
    clouds = job.table(paths_dict['visits_event']) \
        .filter(
            nf.custom(lambda x: str(x) < date, 'event_time')
        ) \
        .put(paths_dict['visits_event'])
    job.run()
    metrika_clickhouse_param_dict['user'] = metrika_creds['value']['login']
    metrika_clickhouse_param_dict['password'] = metrika_creds['value']['pass']

    df_dict = {}
    for key in queries_append_mode:
        metrika_clickhouse_param_dict['query'] = queries_append_mode[key]
        df_dict[key] = clickhouse.get_clickhouse_data(**metrika_clickhouse_param_dict)


    result_df = df_dict['visits_tech_info']
    for key in df_dict:
        if key not in ['visits_tech_info', 'visits_yandexuid_puid_dict']:
            result_df = pd.merge(
                result_df,
                df_dict[key],
                on = ['visit_id', 'visit_version', 'counter_id'],
                how='left'
            )

    result_df = pd.merge(
        result_df,
        df_dict['visits_yandexuid_puid_dict'][['visit_id','puid']],
        on = ['visit_id'],
        how='left'
    )

    result_df = result_df[result_df.groupby(['visit_id', 'counter_id'])['visit_version'].transform(max) == result_df['visit_version']]

    int_columns = [
        'mobile_phone_vendor',
        'income',
        'hits',
        'page_views',
        'duration',
        'total_visits',
        'is_bounce',
        'resolution_width',
        'resolution_height',
        'window_client_width',
        'window_client_height',
        'ad_block',
        'resolution_depth',
    ]
    for col in int_columns:
        try:
            result_df[col] = result_df[col].astype(int)
        except:
            result_df[col] = result_df[col].astype(float)

    result_df['puid'] = result_df.apply(lambda row: row['puid'] if row['puid'] > '0' else row['user_id'], axis = 1)
    cluster.write(paths_dict['visits_event'], result_df.fillna(''), append=True)
if __name__ == '__main__':
    main()
