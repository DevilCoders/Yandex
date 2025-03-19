#!/usr/bin/env python2
# -*- coding: utf-8 -*-
import os, sys, pandas as pd, datetime, telebot
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
from metrika_visits_init_variables import queries,queries_append_mode
from creds import (
    yt_creds,
    metrika_creds,
    yc_ch_creds,
    crm_sql_creds,
    stat_creds
)

from nile.api.v1 import (
    clusters,
    aggregators as na,
    extractors as ne,
    filters as nf,
    Record
)

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

def main():
    date = str(datetime.date.today()-datetime.timedelta(days = 1))

    paths_dict_test = {
        'visits_event':'//home/cloud_analytics_test/cooking_cubes/acquisition_cube/sources/visits'
    }
    paths_dict_prod = {
        'visits_event':'//home/cloud_analytics/cooking_cubes/acquisition_cube/sources/visits'
    }
    mode = 'prod'
    if mode == 'test':
        paths_dict = paths_dict_test
    elif mode == 'prod':
        paths_dict = paths_dict_prod

    schema = {
        "ad_block": int,
        "age": str,
        "area": str,
        "channel": str,
        "channel_detailed": str,
        "city": str,
        "client_ip": str,
        "counter_id": str,
        "country": str,
        "device_model": str,
        "device_type": str,
        "duration": int,
        "event": str,
        "event_time": str,
        "first_visit_dt": str,
        "general_interests": str,
        "hits": int,
        "income": int,
        "interests": str,
        "is_bounce": int,
        "is_robot": str,
        "mobile_phone_vendor": int,
        "os": str,
        "page_views": int,
        "puid": str,
        "referer": str,
        "remote_ip": str,
        "resolution_depth": int,
        "resolution_height": int,
        "resolution_width": int,
        "search_phrase": str,
        "sex": str,
        "start_time": str,
        "start_url": str,
        "total_visits": int,
        "user_id": str,
        "utm_campaign": str,
        "utm_content": str,
        "utm_medium": str,
        "utm_source": str,
        "utm_term": str,
        "visit_id": str,
        "visit_version": str,
        "window_client_height": int,
        "window_client_width": int
    }
    cluster = clusters.yt.Hahn(
        token = yt_creds['value']['token'],
        pool = yt_creds['value']['pool']
    )
    job = cluster.job()
    clouds = job.table(paths_dict['visits_event'] + '_') \
        .filter(
            nf.custom(lambda x: str(x) < date, 'event_time')
        ) \
        .put(paths_dict['visits_event'] + '_')
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

    cluster.write(paths_dict['visits_event']+ '_', result_df.fillna(''), append=True)
    job = cluster.job()
    job.table(paths_dict['visits_event']+'_') \
    .project(
        **apply_types_in_project(schema)
    ) \
    .put(paths_dict['visits_event'], schema = schema)
    job.run()
if __name__ == '__main__':
    main()
