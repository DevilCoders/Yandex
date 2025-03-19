#!/usr/bin/python3
# -*- coding: utf-8 -*-

import datetime
import psycopg2
import requests
import time
import yaml

from dateutil import parser

hbf_lag = 18000
calculate_lag = 1800
rules_lag = 1200
update_lag = 3600
now = datetime.datetime.now()
timestamp_now = int(datetime.datetime.timestamp(now))

try:
    with open("/etc/valve/valve.yaml", 'r') as stream:
        data_loaded = yaml.safe_load(stream)
    hbf_url = data_loaded.get('worker', []).get('hbf_url', '') + '/get/1.1.1.1'
    conn = psycopg2.connect(data_loaded.get('storage', []).get("pg_conn_string", ""))
except:
    print('1;failed to load and parse valve.yaml')
    exit(1)

try:
    r = requests.get(hbf_url)
    date_hbf = r.headers['last-modified']
    timestamp_hbf = int(datetime.datetime.timestamp(parser.parse(date_hbf)))
except:
    print('1;failed to get info from hbf')
    exit(1)

time.sleep(20)

try:
    cursor = conn.cursor()
    cursor.execute("SELECT * FROM rulesversion order by update_ts desc limit 5")
    records = cursor.fetchall()
    rules_ts = int(datetime.datetime.timestamp(records[0][1]))
    update_ts = int(datetime.datetime.timestamp(records[0][2]))
except:
    print('1;failed to fetch data from pgaas')

if (timestamp_now - rules_ts) > hbf_lag:
    print('2;last rules older then 10 hour')
    exit(2)
if (timestamp_hbf - rules_ts) > rules_lag:
    print('2;rules ts older then hbf ts')
    exit(2)
if (update_ts - rules_ts) > update_lag:
    print('2;slow rule update(more then 1 hour)')
    exit(2)

i = 0
for result in records:
    if result[3] == 'CRITICAL':
        i = i + 1
if i == 5:
    print('2;5 CRITICAL in last result')
    exit(2)

if (timestamp_now - timestamp_hbf) > hbf_lag:
    print('1;hbf timestamp older then 10 hour')
    exit(1)

print('0;Ok')

