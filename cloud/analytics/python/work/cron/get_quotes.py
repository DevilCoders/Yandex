#!/usr/bin/env python2
# -*- coding: utf-8 -*-
import pandas as pd, datetime, ast, numpy as np, telebot, json, requests, os, sys, time
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
    Record,
    files as nfi
)
from qb2.api.v1 import (
    filters as qf,
    resources as qr,
    extractors as qe,
    typing as qt,
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
def apply_quotes(records):
    currency_rates = qr.json('currency_rates.json')()

    for record in records:
        for date in currency_rates:
            for curr in currency_rates[date]:
                yield Record(**{'date': date, 'currency': curr, 'quote': currency_rates[date][curr]})

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
    cluster = clusters.yt.Hahn(
        token = yt_creds['value']['token'],
        pool = yt_creds['value']['pool']
    )
    schema = {
        'date': str,
        'currency': str,
        'quote': float
    }
    job = cluster.job()
    job.table('//home/cloud_analytics/import/quotes/__') \
    .map(
        apply_quotes,
        files=[
            nfi.StatboxDict('currency_rates.json', use_latest=True)
        ]
    ) \
    .project(
        **apply_types_in_project(schema)
    ) \
    .put(
        '//home/cloud_analytics/import/quotes/quotes',
        schema = schema
    )
    job.run()
if __name__ == '__main__':
    main()
