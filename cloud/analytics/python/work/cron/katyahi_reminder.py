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
from acquisition_cube_queries import (
    queries_test,
    queries_prod
)

from acquisition_cube_init_funnel_steps import (
    paths_dict_prod,
    paths_dict_test,
    schema_cube as schema
)

def main():
    t = '928316220:AAG-2mwwvwPDgkL8GadsG9yq-gnMNFXwQhI'

    bot = telebot.TeleBot(t)

    text = '''
    Доброе утро, зайки!\nДо 20 июня осталось {0} дней!\nУбедись что ты освободил свой календарь и приготовил агрегаты принимать сырье!\nБУУУУХАААААТЬ!!!!!
    '''.format((datetime.date(2020, 6, 20) - datetime.date.today()).days)
    requests.post('https://api.telegram.org/bot{0}/sendMessage?chat_id={1}&text={2}'.format(
        t,
        '-227185406',
        text
        )
    )

if __name__ == '__main__':
    main()
