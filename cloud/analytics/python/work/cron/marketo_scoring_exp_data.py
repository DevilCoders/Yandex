#!/usr/bin/env python2
# -*- coding: utf-8 -*-
import pandas as pd, numpy as np, catboost, logging, os, sys, requests, datetime, time
from sklearn.model_selection import train_test_split
from sklearn.utils import shuffle
from sklearn.metrics import confusion_matrix, recall_score, precision_score, roc_auc_score
import scipy.stats as stats
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
    stat_creds
)
def wait_for_done_running_proccess(os, file_names):
    #lst = lst.split('\n')
    proccess_running = 0
    files_done_dict = {}
    file_names = file_names.split(',')
    while True:
        for file_name in file_names:
            lst = os.popen('ps -ef | grep python').read()
            if file_name in lst:
                print('%s is running' % (file_name))
                time.sleep(30)
            else:
                files_done_dict[file_name] = 1
                proccess_running = 0
                print('%s Done' % (file_name))

        if len(files_done_dict) == len(file_names):
            break

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

def main():
    files_list = 'marketo_scoring_train_and_apply_model.py'
    wait_for_done_running_proccess(os, files_list)

    mode = 'test'
    cluster = clusters.yt.Hahn(
        token = yt_creds['value']['token'],
        pool = yt_creds['value']['pool']
    )

    job = cluster.job()
    lead_dataset = get_table_list('//home/cloud_analytics/scoring/leads', job)

    schema = {
        "billing_account_id": str,
        "first_trial_consumption_date": str,
        "group": str,
        "paid_prob": float,
        "prob_cat": str,
        "prob_threshold": int,
        "puid": str,
        "score": float,
        "score_cat": str,
    }
    job = cluster.job()
    job.table(lead_dataset) \
    .project(
        **apply_types_in_project(schema)
    ) \
    .put(
        '//home/cloud_analytics/scoring/reports/leads',
        schema = schema
    )
    job.run()

    schema = {
        "date": str,
        "precision": float,
        "recall": float,
        "roc_auc": float
    }
    job = cluster.job()
    job.table('//home/cloud_analytics/scoring/model_quality/metrics') \
    .project(
        **apply_types_in_project(schema)
    ) \
    .put(
        '//home/cloud_analytics/scoring/reports/model_quality',
        schema = schema
    )
    job.run()

    job = cluster.job()
    #test_dataset = get_table_list('//home/cloud_analytics/scoring/model_quality/test_dataset', job)
    temp = cluster.read('//home/cloud_analytics/scoring/model_quality/test_dataset/test_dataset').as_dataframe()
    schema = {}
    for col in temp.columns:
        if isinstance(temp[col].iloc[0], int):
            schema[col] = int
        elif isinstance(temp[col].iloc[0], float):
            schema[col] = float
        else:
            schema[col] = str

    job = cluster.job()
    job.table('//home/cloud_analytics/scoring/model_quality/test_dataset/test_dataset') \
    .project(
        **apply_types_in_project(schema)
    ) \
    .put(
        '//home/cloud_analytics/scoring/reports/test_sets',
        schema = schema
    )
    job.run()

    job = cluster.job()
    #train_dataset = get_table_list('//home/cloud_analytics/scoring/model_quality/train_dataset', job)
    temp = cluster.read('//home/cloud_analytics/scoring/model_quality/train_dataset/train_dataset').as_dataframe()
    schema = {}
    for col in temp.columns:
        if isinstance(temp[col].iloc[0], int):
            schema[col] = int
        elif isinstance(temp[col].iloc[0], float):
            schema[col] = float
        else:
            schema[col] = str

    job = cluster.job()
    job.table('//home/cloud_analytics/scoring/model_quality/train_dataset/train_dataset') \
    .project(
        **apply_types_in_project(schema)
    ) \
    .put(
        '//home/cloud_analytics/scoring/reports/train_sets',
        schema = schema
    )
    job.run()

    job = cluster.job()
    #scored_users = get_table_list('//home/cloud_analytics/scoring/model_quality/scored_users', job)
    temp = cluster.read('//home/cloud_analytics/scoring/model_quality/scored_users/scored_users').as_dataframe()
    schema = {}
    for col in temp.columns:
        if isinstance(temp[col].iloc[0], int):
            schema[col] = int
        elif isinstance(temp[col].iloc[0], float):
            schema[col] = float
        else:
            schema[col] = str
    job = cluster.job()
    job.table('//home/cloud_analytics/scoring/model_quality/scored_users/scored_users') \
    .project(
        **apply_types_in_project(schema)
    ) \
    .put(
        '//home/cloud_analytics/scoring/reports/scored_users',
        schema = schema
    )
    job.run()

if __name__ == '__main__':
    main()
