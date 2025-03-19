# -*- coding=utf-8 -*-
from __future__ import print_function
from __future__ import absolute_import
from __future__ import unicode_literals

import logging
from pprint import pprint
import datetime

from yql.api.v1.client import YqlClient
import yt.yson as yson
import yt.wrapper as yt


def create_agents_location_table(tors, agents):
    rows = []
    # save 'site' as 'module'
    names = ['agent_ip', 'agent_name', 'rack', 'module', 'building', 'dc']
    types = ['Utf8', 'Utf8', 'Utf8', 'Utf8', 'Utf8', 'Utf8']
    for k, v in agents.items():
        agent_ip = k
        agent_name = v['name']
        rows.append([agent_ip,
                     agent_name,
                     tors[agent_name]['rack'],
                     tors[agent_name]['site'],
                     tors[agent_name]['building'],
                     tors[agent_name]['dc']
                     ])
    # DIRTY HACK: Добавим в самый конец описание для префикса Яндекса как еще
    # одного дата центра. Мы не будем добавлять его в таблицу с префиксами,
    # т.к. нам надо гарантировать, что проверка будет последней и до этого
    # совпадений не было. Проверка вынесем в отдельное условие внутри
    # yql $lookup.
    rows.append(["YANDEX", # agent_ip
                "YANDEX", # agent_name
                "", # rack
                "", # site
                "", # building
                "YANDEX", # dc
                ])
    return rows, names, types


def create_prefixes_table(prefixes):
    rows = []
    names = ['prefix', 'agent_name']
    types = ['Utf8', 'Utf8']
    for prefix in prefixes.keys():
        agent_name = prefixes[prefix]['name']
        rows.append([prefix,
                     agent_name
                     ])
    return rows, names, types


def update_netbox_tables(tors, agents, prefixes):
    """
    tors example:
    {'cloud-vla1-4s7.yndx.net': {'building': 'ALPHA',
                             'dc': 'VLADIMIR',
                             'rack': '4A48',
                             'site': 'VLA-04'},
    'cloud-vla1-4s9.yndx.net': {'building': 'ALPHA',
                             'dc': 'VLADIMIR',
                             'rack': '4B43',
                             'site': 'VLA-04'}}
    agents examples:
    {'172.16.2.128': {'name': 'cloud-sas2-92s1.yndx.net'},
     '172.16.2.129': {'name': 'cloud-sas2-92s3.yndx.net'},
     '172.16.2.130': {'name': 'cloud-sas2-92s5.yndx.net'}}

    prefixes example:
     {'2a02:6b8:c0e:60a::/64': {'name': 'cloud-vla1-4s21.yndx.net'},
      '2a02:6b8:c0e:60b::/64': {'name': 'cloud-vla1-4s25.yndx.net'}}
    """

    logging.info(f"Updating netbox infromation in YT")
    client = YqlClient(db='hahn', token_path='/Users/atsygane/.yql/token')

    logging.info(f"Creating table with locations")
    locations_table = 'home/cloud-netinfra/netbox/locations'
    client.write_table(locations_table, *create_agents_location_table(tors, agents))

    logging.info(f"Creating table with prefixes")
    prefixes_table = 'home/cloud-netinfra/netbox/prefixes'
    client.write_table(prefixes_table, *create_prefixes_table(prefixes))

    logging.info(f"Updating netbox information in YT is done")


def process_daily_aggregation(date):
  query = ""
  values = {}
  values['year'] = date[0:4]
  values['month'] = date[5:7]
  values['day'] = date[8:10]

  with open('aggregate_daily_flows.yql', 'r') as file:
    query = file.read()
  # print(query.format(**values))
  query = query % values
  title = 'automated q1 {} YQL'.format(date)
  client = YqlClient(db='hahn', token_path='/Users/atsygane/.yql/token')
  request =client.query(query, syntax_version=1, title=title)
  request.run()


def yql_example_please_ignore():
    client = YqlClient(db='hahn', token_path='/Users/atsygane/.yql/token')

    request = client.query(
        '''SELECT Path FROM FOLDER("logs/cloud-netinfra-sflow-log/1d", "schema;row_count")
            WHERE Type = "table" AND Yson::GetLength(Attributes.schema) > 0
                                AND Yson::LookupInt64(Attributes, "row_count") > 0
            ORDER BY Path;
        ''')
    request.run()

    # print(request.json)
    for table in request.get_results():  # access to results blocks until they are ready
        table.fetch_full_data()  # see https://nda.ya.ru/3S3j25
        print('=== Table ===')
        print('--- Schema ---')
        for column_name, column_print_type in zip(table.column_names, table.column_print_types):
            print(column_name + ': ' + column_print_type)

        print('\n--- Data ---')
        for row in table.rows:
            print('\t'.join([str(cell) for cell in row]))


def is_calculation_proceed_required(date):
    client = YqlClient(db='hahn', token_path='/Users/atsygane/.yql/token')

    request = client.query(
        '''SELECT Path FROM FOLDER("logs/cloud-netinfra-sflow-log/1d", "schema;row_count")
            WHERE Type = "table" AND Yson::GetLength(Attributes.schema) > 0
                                AND Yson::LookupInt64(Attributes, "row_count") > 0
            ORDER BY Path DESC
            LIMIT 1;

            SELECT Path FROM FOLDER("home/cloud-netinfra/sflow/1d", "schema;row_count")
            WHERE Type = "table" AND Yson::GetLength(Attributes.schema) > 0
                                AND Yson::LookupInt64(Attributes, "row_count") > 0
            ORDER BY Path DESC
            LIMIT 1;
        ''')
    request.run()

    last_tables=['', '']
    index = 0
    for table in request.get_results():  # access to results blocks until they are ready
        table.fetch_full_data()  # see https://nda.ya.ru/3S3j25
        for row in table.rows:
            table_date = row[0][-10:]
            last_tables[index]= table_date
        index += 1

    if (len(last_tables) == 2) and (last_tables[0] == last_tables[1] == date):
      logging.info(f"Already have aggregates for yesterday {date}")
      return False
    elif((last_tables[0]=='') or (last_tables[0]!=date)):
      logging.info(f"We don't have logs for {date} yet")
      return False

    return True


def missing_dates(input_dates):
    input_dates.sort()
    dates=[]
    for d in input_dates:
        dates.append(datetime.datetime.strptime(d, '%Y-%m-%d'))
    dates_set = set(dates[0] + datetime.timedelta(x) for x in range((dates[-1] - dates[0]).days))
    misses = dates_set - set(dates)
    return misses


def is_calculation_proceed_required_yt_version():
  client = yt.YtClient("hahn")
  aggregated_logs = []
  data = client.list("//home/cloud-netinfra/sflow/1d", attributes=["schema", "row_count"])
  for entry in data:
      if len(entry.attributes['schema'])>0 and entry.attributes['row_count']>0:
          # print(type(entry))
          aggregated_logs.append(str(entry))
  print(missing_dates(aggregated_logs))
  return True
