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

def get_last_table(folder_path, job):
    tables_list = sorted([folder_path + '/' + x for x in job.driver.list(folder_path)], reverse=True)
    if tables_list:
        return tables_list[0]


def get_table_list_without_last(folder_path,job):
    tables_list = sorted([folder_path + '/' + x for x in job.driver.list(folder_path)], reverse=True)
    if len(tables_list) >= 2:
        tables_list = tables_list[1:]
        return '{%s}' % (','.join(tables_list))
    return None

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
    folders_to_clean = [
        '//home/cloud_analytics/import/crm/accounts',
        '//home/cloud_analytics/import/crm/opportunities',
        '//home/cloud_analytics/import/crm/calls',
        '//home/cloud_analytics/import/crm/contacts',
        '//home/cloud_analytics/import/crm/billing_accounts',
        '//home/cloud_analytics/import/crm/dimensions'
    ]

    mode = 'test'
    cluster = clusters.yt.Hahn(
        token = yt_creds['value']['token'],
        pool = yt_creds['value']['pool']
    )

    for folder in folders_to_clean:
        job = cluster.job()
        last_table = get_last_table(folder, job)

        other_tables = get_table_list_without_last(folder, job)
        if other_tables:
            tables_to_delete = other_tables.replace('{', '').replace('}', '').split(',')

            job = cluster.job()
            job.table(other_tables) \
            .project(ne.all(exclude = '_other')) \
            .put(last_table, append = True)
            job.run()

            for table in tables_to_delete:
                cluster.driver.remove(table)
if __name__ == '__main__':
    main()
