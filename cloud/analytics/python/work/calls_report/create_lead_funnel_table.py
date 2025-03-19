#!/usr/bin/env python2
# -*- coding: utf-8 -*-
import pandas as pd, datetime, ast, os,sys, pymysql, time
module_path = os.path.abspath(os.path.join('/home/ktereshin/yandex/arcadia/cloud/analytics/python/work'))
if module_path not in sys.path:
    sys.path.append(module_path)
from data_loader import clickhouse
from yt.transfer_manager.client import TransferManager
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
from vault_client import instances

def works_with_emails(mail_):
    mail_parts = str(mail_).split('@')
    if len(mail_parts) > 1:
        if 'yandex.' in mail_parts[1].lower() or 'ya.' in mail_parts[1].lower():
            domain = 'yandex.ru'
            login = mail_parts[0].lower().replace('.', '-')
            return login + '@' + domain
        else:
            return str(mail_).lower()
    else:
        return str(mail_).lower()

def get_last_not_empty_table(folder_path):
    tables_list = sorted([folder_path + '/' + x for x in job.driver.list(folder_path)], reverse=True)
    last_table_rows = 0
    last_table = ''
    for table in tables_list:
        try:
            table_ = job.driver.read(table)
        except:
            continue

        if table_.row_count > last_table_rows:
            last_table_rows =  table_.row_count
            last_table = table
    if last_table:
        return last_table
    else:
        return tables_list[0]

def apply_types_in_project(schema_):
    apply_types_dict = {}
    for col in schema_:

        if schema_[col] == str:
            apply_types_dict[col] = ne.custom(lambda x: str(x) if x not in ['', None] else None, col)

        elif schema_[col] == int:
            apply_types_dict[col] = ne.custom(lambda x: int(x) if x not in ['', None] else None, col)

        elif schema_[col] == float:
            apply_types_dict[col] = ne.custom(lambda x: float(x) if x not in ['', None] else None, col)
    return apply_types_dict

def exucute_query(query, hosts):
    for host in hosts.split(','):
        cloud_clickhouse_param_dict['host'] = host
        cloud_clickhouse_param_dict['query'] = query
        clickhouse.get_clickhouse_data(**cloud_clickhouse_param_dict)


def get_lead_id(email_lead_id, ba_lead_id, puid_lead_id):
    if ba_lead_id:
        return ba_lead_id
    if puid_lead_id:
        return puid_lead_id
    if email_lead_id:
        return email_lead_id

def exucute_query(query, hosts, cloud_clickhouse_param_dict_):
    for host in hosts.split(','):
        try:
            print('start on host: %s' % (host))
            cloud_clickhouse_param_dict_['host'] = host
            cloud_clickhouse_param_dict_['query'] = query
            clickhouse.get_clickhouse_data(**cloud_clickhouse_param_dict_)
            print('============================\n\n')
        except:
            break

def main():
    client = instances.Production()
    yt_creds = client.get_version('ver-01d33pgv8pzc7t99s3egm24x47')
    crm_sql_creds = client.get_version('ver-01d3ktedjm6ptsvwf1xq161hwk')
    metrika_creds = client.get_version('ver-01d2z36msatt9mp9pcfptezksp')
    yc_ch_creds = client.get_version('ver-01d2z39xj02xw7gqvv9wq757ne')

    cluster = clusters.yt.Hahn(
        token = yt_creds['value']['token'],
        pool = yt_creds['value']['pool']
    )
    metrika_clickhouse_param_dict['user'] = metrika_creds['value']['login']
    metrika_clickhouse_param_dict['password'] = metrika_creds['value']['pass']

    cloud_clickhouse_param_dict['user'] = yc_ch_creds['value']['login']
    cloud_clickhouse_param_dict['password'] = yc_ch_creds['value']['pass']


    hosts = 'https://sas-tt9078df91ipro7e.db.yandex.net:8443/,https://vla-2z4ktcci90kq2bu2.db.yandex.net:8443/'
    table = 'cloud_analytics_testing.crm_lead_cube_test_tm'

    query = '''
    CREATE TABLE cloud_analytics_testing.lead_funnel_temp
    ENGINE = MergeTree()
    ORDER BY(time) PARTITION BY toYYYYMM(time)
    AS
    SELECT
      lead_id,
      lead_email,
      billing_account_id,
      puid,
      lead_source,
      ba_person_type,
      ba_state,
      ba_usage_status,
      block_reason,
      sales_name,
      groupArray(lead_state) AS events,
      groupArray(event_time) AS events_times,
      events[1] as first_event,
      arrayMap(
        x -> (x > toDate('2017-01-01')),
        groupArray(event_time)
      ) AS events_times_index,
      arraySum(
        arrayMap(x -> (x = 'call'), groupArray(lead_state))
      ) AS calls,
      events[1] as first_state,
      groupArray(trial_comsumption_total) AS trial_consumption_totals,
      groupArray(paid_comsumption_total) AS paid_consumption_totals,
      arrayFilter(
        (t, name) -> (name = 'New'),
        events_times,
        events
      ) [1] AS time,

      arrayFilter(
        (events_times_index, t, name) -> (name = 'New'),
        events_times_index,
        events_times,
        events
      ) [1] AS new,

      arrayFilter(
        (t, name) -> (name = 'New'),
        events_times,
        events
      ) [1] AS new_time,

      arrayFilter(
        (events_times_index, t, name) -> (
          (name = 'Assigned')
          AND (t >= new_time)
          AND (new != 0)
        ),
        events_times_index,
        events_times,
        events
      ) [1] AS new_assigned,

      arrayFilter(
        (t, name) -> (
          (name = 'Assigned')
          AND (t >= new_time)
          AND (new != 0)
        ),
        events_times,
        events
      ) [1] AS new_assigned_time,

      arrayFilter(
        (events_times_index, t, name) -> (
          (name = 'In Process')
          AND (t >= new_assigned_time)
          AND (new_assigned != 0)
        ),
        events_times_index,
        events_times,
        events
      ) [1] AS assigned_inprocess,

      arrayFilter(
        (t, name) -> (
          (name = 'In Process')
          AND (t >= new_assigned_time)
          AND (new_assigned != 0)
        ),
        events_times,
        events
      ) [1] AS assigned_inprocess_time,

      arrayFilter(
        (events_times_index, t, name) -> (
          (name = 'Recycled')
          AND (t >= assigned_inprocess_time)
          AND (assigned_inprocess != 0)
        ),
        events_times_index,
        events_times,
        events
      ) [1] AS inprocess_recycled,

      arrayFilter(
        (events_times_index, t, name) -> (
          (name = 'first_paid_consumption')
          AND (t >= assigned_inprocess_time)
          AND (assigned_inprocess != 0)
        ),
        events_times_index,
        events_times,
        events
      ) [1] AS inprocess_paid_consumption,

      arrayFilter(
        (events_times_index, t, name) -> (
          (name = 'Converted')
          AND (t >= assigned_inprocess_time)
          AND (assigned_inprocess != 0)
        ),
        events_times_index,
        events_times,
        events
      ) [1] AS inprocess_converted,

      arrayFilter(
        (t, name) -> (
          (name = 'Converted')
          AND (t >= assigned_inprocess_time)
          AND (assigned_inprocess != 0)
        ),
        events_times,
        events
      ) [1] AS inprocess_converted_time,

      arrayFilter(
        (events_times_index, t, name) -> (
          (name = 'first_paid_consumption')
          AND (t >= new_assigned)
          AND (assigned_inprocess != 0)
          AND (inprocess_recycled = 0)
        ),
        events_times_index,
        events_times,
        events
      ) [1] AS converted_paid_consumption,

      arrayFilter(
        (events_times_index, t, name) -> (
          (name = 'first_paid_consumption')
          AND (t >= inprocess_recycled)
          AND (inprocess_recycled != 0)
        ),
        events_times_index,
        events_times,
        events
      ) [1] AS recycled_paid_consumption
    FROM
      (
        SELECT
          *
        FROM
          cloud_analytics_testing.crm_lead_cube_test
        ORDER BY
          event_time ASC,
          lead_state DESC
      )
    GROUP BY
      lead_id,
      lead_email,
      billing_account_id,
      puid,
      lead_source,
      ba_person_type,
      ba_state,
      ba_usage_status,
      block_reason,
      sales_name
    '''

    exucute_query(query, hosts, cloud_clickhouse_param_dict)

    query = '''
    DROP TABLE IF EXISTS cloud_analytics_testing.lead_funnel
    '''

    exucute_query(query, hosts, cloud_clickhouse_param_dict)

    query = '''
    RENAME TABLE cloud_analytics_testing.lead_funnel_temp TO cloud_analytics_testing.lead_funnel
    '''

    exucute_query(query, hosts, cloud_clickhouse_param_dict)

    query = '''
    DROP TABLE IF EXISTS cloud_analytics_testing.lead_funnel_temp
    '''

    exucute_query(query, hosts, cloud_clickhouse_param_dict)

if __name__ == '__main__':
    main()
