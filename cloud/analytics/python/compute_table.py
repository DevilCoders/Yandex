#!/usr/bin/env python2
# -*- coding: utf-8 -*-
"""
Created on Tue Dec 11 16:50:11 2018

@author: artkaz

Код для таблицы Nodes CPU-Mem Load на дашборде Compute Stats в графане
"""
#%%
from SwissArmyKnife import SwissArmyKnife, YTClient, ClickHouseClient
import pandas as pd

yt_client = YTClient('Hahn')

compute_stats = yt_client.read_yt_table('/home/cloud_analytics/compute_stats/2018-12-10')

ch_settings_df = pd.read_excel('/Users/artkaz/Documents/yacloud/settings/ch_cloud_analytics_connection.xlsx') #sys.argv[1]

ch_settings = {ch_settings_df.iloc[i,:]['field']: ch_settings_df.iloc[i,:]['value'] for i in ch_settings_df.index}


ch_client = ClickHouseClient(user = ch_settings['user'],
                             passw = ch_settings['password'],
                             verify = ch_settings['verify'],
                             host = ch_settings['host']
                             )






#%%

compute_stats = compute_stats[compute_stats['date'] > '2018-12-01 00:00:00']
compute_stats_flat = pd.DataFrame(columns=['date', 'node_name', 'node_type', 'node_mem_type', 'zone_id', 'agg'])

agg_set = {'min','avg','max'}

for metric in compute_stats['metric'].drop_duplicates():
    res = pd.DataFrame()
    for agg in agg_set:
        compute_stats_ = compute_stats[compute_stats['metric'] == metric].rename({agg: metric}, axis=1).drop(['metric'] + list(agg_set.difference({agg})), axis=1)
        compute_stats_['agg'] = agg
        res = res.append(compute_stats_)
    compute_stats_flat = pd.merge(compute_stats_flat, res, on=['date', 'node_name', 'node_type', 'node_mem_type', 'zone_id', 'agg'], how='right')

compute_stats_flat.drop(['nvme_total', 'nvme_free'], axis=1, inplace=True)

compute_stats_flat['cores_used_pct'] = compute_stats_flat['cores_used'] / compute_stats_flat['cores_total']
compute_stats_flat['memory_used_pct'] = compute_stats_flat['memory_used'] / compute_stats_flat['memory_total']

import math
import numpy as np
def upper_decimal(x):
    return math.ceil(x*10) / 10. if x>0 else .1


compute_stats_flat['cores_used_pct_bucket'] = compute_stats_flat['cores_used_pct'].apply(upper_decimal)
compute_stats_flat['memory_used_pct_bucket'] = compute_stats_flat['memory_used_pct'].apply(upper_decimal)


#%%
compute_stats_grouped = compute_stats_flat.groupby(['agg',
                                                    'date',
                                                    'node_type',
                                                    'node_mem_type',
                                                    'zone_id',
                                                    'cores_used_pct_bucket',
                                                    'memory_used_pct_bucket'], as_index=False).agg({'node_name':'nunique'}).rename({'node_name': 'node_count'}, axis=1)



compute_stats_pivot = pd.pivot_table(compute_stats_grouped, index = ['agg',
                                                                     'date',
                                                                     'node_type',
                                                                     'node_mem_type',
                                                                     'zone_id',
                                                                     'cores_used_pct_bucket'],
                                        columns = ['memory_used_pct_bucket'], values=['node_count']).reset_index().fillna(0)

compute_stats_pivot.columns = ['agg',
                               'date',
                               'node_type',
                               'node_mem_type',
                               'zone_id',
                               'cores_used_pct_bucket',
                               'memory_10pct',
                               'memory_20pct',
                               'memory_30pct',
                               'memory_40pct',
                               'memory_50pct',
                               'memory_60pct',
                               'memory_70pct',
                               'memory_80pct',
                               'memory_90pct',
                               'memory_100pct', ]


def return_all_combinations(df):
    d = {}
    for col in df.columns:
        df_ = df[[col]].drop_duplicates()
        df_['a'] = 1
        d[col] =df_
    res = pd.DataFrame(columns=['a'])
    for k in d.keys():
        res = pd.merge(res, d[k], on='a', how='outer')
    return res.drop('a', axis=1)

compute_stats_all_combs = return_all_combinations(compute_stats_pivot[['agg',
                               'date',
                               'node_type',
                               'node_mem_type',
                               'zone_id',
                               'cores_used_pct_bucket', ]])


compute_stats_pivot = pd.merge(compute_stats_all_combs, compute_stats_pivot, on = ['agg',
                               'date',
                               'node_type',
                               'node_mem_type',
                               'zone_id',
                               'cores_used_pct_bucket', ], how='left').fillna(0)

compute_stats_pivot['cores_used_pct_bucket'] = compute_stats_pivot['cores_used_pct_bucket'].apply(lambda b: 'abcdefghijk'[int(b*10)-1] +'.cpu_' + str(int(b*100)) + 'pct')

#%%
ch_client.write_df_to_table(compute_stats_pivot, 'cloud_analytics.cpu_x_memory')


#%%
compute_stats_grouped['cores_used_pct_bucket'] = compute_stats_grouped['cores_used_pct_bucket'].apply(lambda b: 'abcdefghijk'[int(b*10)-1] +'.cpu_' + str(int(b*100)) + 'pct')
compute_stats_grouped['memory_used_pct_bucket'] = compute_stats_grouped['memory_used_pct_bucket'].apply(lambda b: 'abcdefghijk'[int(b*10)-1] +'.mem_' + str(int(b*100)) + 'pct')

#%%
ch_client.write_df_to_table(compute_stats_grouped, 'cloud_analytics.cpu_x_memory_flat')

