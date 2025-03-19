#!/usr/bin/env python2
# -*- coding: utf-8 -*-
import pandas as pd, datetime, ast, numpy as np, telebot, json, requests, os, sys
module_path = os.path.abspath(os.path.join('/home/ktereshin/yandex/arcadia/cloud/analytics/python/work'))
if module_path not in sys.path:
    sys.path.append(module_path)
from nile.api.v1 import (
    clusters,
    aggregators as na,
    extractors as ne,
    filters as nf,
    Record
)
from vault_client import instances

from init_funnel_steps import (
    paths_dict_prod,
    paths_dict_test
)

def get_service(name_):
    return str(name_).split('.')[0]


def convert_metric_to_float(num):
    try:
        return float(num)
    except:
        return 0.0


def get_payment_type(context):
    try:
        return ast.literal_eval(context)['payment_type']
    except:
        return None

def get_reason(metadata_):
    if metadata_ not in ['', None]:
        metadata =  json.loads(metadata_)
    else:
        metadata = None

    if metadata:
        if 'block_reason' in metadata:
            return metadata['block_reason']
        if 'suspend_reason' in metadata:
            return metadata['suspend_reason']
        #if 'fraud_detected_by' in metadata:
            #if isinstance(metadata['fraud_detected_by'], list):
                #return metadata['fraud_detected_by'][0]
            #else:
                #return metadata['fraud_detected_by'].replace('[', '').replace(']', '').replace("u'", '').replace("'", '')
    return 'unknown'

def get_datetime_from_epoch(epoch):
    try:
        return str(datetime.datetime.fromtimestamp(int(epoch)))
    except:
        return None


def convert_epoch_to_end_day(epoch):
    try:
        return str(datetime.datetime.fromtimestamp(int(epoch)).replace(hour=23).replace(minute=59).replace(second=59))
    except:
        return None

def date_range_by_days(start_str, end_str):
    start = datetime.datetime.strptime(start_str, '%Y-%m-%d')
    end = datetime.datetime.strptime(end_str, '%Y-%m-%d')
    delta = int((end - start).days) + 1
    date_list = []

    for i in range(delta):
        date_list.append( str((start + datetime.timedelta(days = i)).date()) )
    return date_list

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

def convert_epoch_to_end_day(epoch):
    try:
        return str(datetime.datetime.fromtimestamp(int(epoch)).replace(hour=23).replace(minute=59).replace(second=59))
    except:
        return None

def convert_epoch_to_date(epoch):
    try:
        return str(datetime.datetime.fromtimestamp(int(epoch)).date())
    except:
        return None

def date_range_by_days(start_str, end_str):
    start = datetime.datetime.strptime(start_str, '%Y-%m-%d')
    end = datetime.datetime.strptime(end_str, '%Y-%m-%d')
    delta = int((end - start).days) + 1
    date_list = []

    for i in range(delta):
        date_list.append( str((start + datetime.timedelta(days = i)).date()) )

    return date_list


def get_ba_history(groups):
    for key, records in groups:

        rec_list = list(records)
        result_dict = {}
        start_date = ''
        owner_id = ''
        for rec in rec_list:
            rec_ = rec.to_dict()
            rec_['date'] = convert_epoch_to_date(rec_['updated_at'])
            if not start_date:
                start_date = rec_['date']

            if 'owner_id' in rec_:
                owner_id = rec_['owner_id']

            result_dict[rec_['date']] = rec_



        date_range = date_range_by_days(start_date, str(datetime.date.today()))
        res = {}
        for date in date_range:

            for date_ in sorted(result_dict):

                if date_ <= date:
                    result_dict[date_]['date'] = date
                    res = result_dict[date_]
                else:
                    break
            res['owner_id'] = owner_id
            yield Record(key, **res)


def main():
    mode = 'test'
    client = instances.Production()
    yt_creds = client.get_version('ver-01d33pgv8pzc7t99s3egm24x47')
    telebot_creds = client.get_version('ver-01d4wza60ns43j5mqktvvvwdnx')
    bot = telebot.TeleBot(telebot_creds['value']['token'])
    cluster = clusters.yt.Hahn(
        token = yt_creds['value']['token'],
        pool = yt_creds['value']['pool']
    )
    if mode == 'test':
        paths_dict_temp = paths_dict_test
    elif mode == 'prod':
        paths_dict_temp = paths_dict_prod
    paths_dict = paths_dict_temp.copy()

    job = cluster.job()
    for path in['billing_accounts_history_path']:
        valid_path = get_last_not_empty_table(paths_dict_temp[path], job)
        paths_dict[path] = valid_path

    schema = {
        "balance": float,
        "balance_client_id": str,
        "balance_contract_id": str,
        "billing_account_id": str,
        "billing_threshold": str,
        "client_id": str,
        "currency": str,
        "date": str,
        "feature_flags": str,
        "iso_eventtime": str,
        "master_account_id": str,
        "metadata": str,
        "name": str,
        "owner_id": str,
        "payment_cycle_type": str,
        "payment_method_id": str,
        "payment_type": str,
        "person_type": str,
        "state": str,
        "type": str,
        "updated_at": int,
        "usage_status": str,
        "block_reason": str
    }

    job = cluster.job()
    job.table(paths_dict['billing_accounts_history_path']) \
    .project(
        ne.all(),
        block_reason = ne.custom(lambda x,y:get_reason(y) if x == 'suspended' else 'Unlocked', 'state', 'metadata')
    ) \
    .groupby(
        'billing_account_id'
    ) \
    .sort(
        'updated_at'
    ) \
    .reduce(
        get_ba_history
    ) \
    .project(
        **apply_types_in_project(schema)
    ) \
    .put(paths_dict['ba_hist'], schema = schema)
    job.run()

if __name__ == '__main__':
    main()
