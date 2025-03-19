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
    telebot_creds,
    wiki_creds
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
    api_url = 'https://st-api.yandex-team.ru/v2/issues/'
    headers = {
        'Authorization': "OAuth %s" % wiki_creds['value']['token']
    }
    additional_tasks = [
        'CLOUDBIZ-2564',
        'CLOUDBIZ-1252'
    ]

    tasks_in_queue = 'https://st-api.yandex-team.ru/v2/issues?filter=queue:CLOUDBIZ&filter=author:annapesh&perPage=10000'
    r = requests.get(tasks_in_queue, headers=headers)
    tasks_info = json.loads(r.text)
    tasks = sorted([str(task['key']) for task in tasks_info])
    tasks = tasks + additional_tasks
    results = []
    for task in tasks:
        tasks_in_queue = 'https://st-api.yandex-team.ru/v2/issues/%s' % (task)
        r = requests.get(tasks_in_queue, headers=headers)
        tasks_info = json.loads(r.text)
        if 'запрос на грант' in tasks_info['summary'].lower().encode('utf-8'):
            results.append(tasks_info)


    res_df = pd.DataFrame(results)[['key',  'summary', 'createdAt']].rename(columns  = {'key':'task_key', 'summary':'task_title', 'createdAt':'created_at'})
    res_df['created_at'] = res_df['created_at'].apply(lambda x: str(x).split('.')[0].replace('T', ' '))

    schema = {
        'task_key': str,
        'task_title': str,
        'created_at': str
    }
    cluster_yt = clusters.yt.Hahn(
        token = yt_creds['value']['token'],
        pool = yt_creds['value']['pool']
    )
    cluster_yt.write('//home/cloud_analytics/export/startrack/events_promocodes_',res_df)

    job = cluster_yt.job()
    job.table('//home/cloud_analytics/export/startrack/events_promocodes_') \
    .project(
        **apply_types_in_project(schema)
    ) \
    .put('//home/cloud_analytics/export/startrack/events_promocodes', schema = schema)
    job.run()
    cluster_yt.driver.remove('//home/cloud_analytics/export/startrack/events_promocodes_')
if __name__ == '__main__':
    main()
