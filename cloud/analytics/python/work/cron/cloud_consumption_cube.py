#!/usr/bin/env python2
# -*- coding: utf-8 -*-
import pandas as pd, datetime, ast, os,sys, pymysql, time
module_path = os.path.abspath(os.path.join('/home/ktereshin/yandex/arcadia/cloud/analytics/python/work'))
if module_path not in sys.path:
    sys.path.append(module_path)
module_path = os.path.abspath(os.path.join('/home/ktereshin/yandex/cron'))
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
from creds import (
    yt_creds,
    metrika_creds,
    yc_ch_creds,
    crm_sql_creds,
    stat_creds,
    telebot_creds
)

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

    cluster = clusters.yt.Hahn(
        token = yt_creds['value']['token'],
        pool = yt_creds['value']['pool']
    )
    metrika_clickhouse_param_dict['user'] = metrika_creds['value']['login']
    metrika_clickhouse_param_dict['password'] = metrika_creds['value']['pass']

    cloud_clickhouse_param_dict['user'] = yc_ch_creds['value']['login']
    cloud_clickhouse_param_dict['password'] = yc_ch_creds['value']['pass']


    hosts = 'https://sas-tt9078df91ipro7e.db.yandex.net:8443/,https://vla-2z4ktcci90kq2bu2.db.yandex.net:8443/'

    query = '''
    CREATE TABLE cloud_analytics.cloud_consumption_cube_temp
    ENGINE = MergeTree()
    ORDER BY(date) PARTITION BY toYYYYMM(date)
    AS
    SELECT
        assumeNotNull(date) date,
        assumeNotNull(t1.billing_account_id) billing_account_id,
        assumeNotNull(multiIf(ba_curr_state IS NULL, 'unknown', ba_curr_state)) ba_curr_state,
        assumeNotNull(multiIf(ba_curr_state_extended IS NULL, 'unknown', ba_curr_state_extended)) ba_curr_state_extended,
        assumeNotNull(multiIf(person_type IS NULL, 'unknown', person_type)) person_type,
        assumeNotNull(multiIf(service1 IS NULL, 'unknown', service1)) service1,
        assumeNotNull(multiIf(service2 IS NULL, 'unknown', service2)) service2,
        assumeNotNull(multiIf(service3 IS NULL, 'unknown', service3)) service3,
        assumeNotNull(multiIf(service4 IS NULL, 'unknown', service4)) service4,
        assumeNotNull(multiIf(service5 IS NULL, 'unknown', service5)) service5,
        assumeNotNull(multiIf(sku IS NULL, 'unknown', sku)) sku,
        assumeNotNull(multiIf(usage_status IS NULL, 'unknown', usage_status)) usage_status,
        assumeNotNull(multiIf(source IS NULL, 'unknown', source)) source,
        assumeNotNull(multiIf(source2 IS NULL, 'unknown', source2)) source2,
        assumeNotNull(multiIf(segment IS NULL, 'unknown', segment)) segment,
        assumeNotNull(multiIf(sales_person IS NULL, 'unknown', sales_person)) sales_person,
        assumeNotNull(multiIf(idc_industry IS NULL, 'unknown', idc_industry)) idc_industry,
        assumeNotNull(multiIf(channel IS NULL, 'unknown', channel)) channel,
        assumeNotNull(cloud_id) cloud_id,
        assumeNotNull(cost) cost,
        assumeNotNull(credit) credit,
        assumeNotNull(paid_consumption) paid_consumption,
        assumeNotNull(offer_amount) offer_amount,
        assumeNotNull(client_name) client_name
    FROM(
        SELECT
            event_time as date,
            billing_account_id,
            cloud_id,
            splitByString('.', assumeNotNull(name))[1] as service1,
            splitByString('.', assumeNotNull(name))[2] as service2,
            splitByString('.', assumeNotNull(name))[3] as service3,
            splitByString('.', assumeNotNull(name))[4] as service4,
            splitByString('.', assumeNotNull(name))[5] as service5,
            name as sku,
            real_consumption + trial_consumption as cost,
            -1*trial_consumption as credit,
            real_consumption as paid_consumption,
            grant_amount as offer_amount,
            'unknown' as source,
            'unknown' as source2,
            'unknown' as idc_industry
        FROM cloud_analytics.acquisition_cube
        WHERE
            event = 'day_use'
    ) as t0
    ANY LEFT JOIN (
        SELECT
            billing_account_id,
            ba_state as ba_curr_state,
            multiIf(ba_state = 'suspended',CONCAT(lower(ba_state), '_', lower(block_reason)), ba_state) as ba_curr_state_extended,
            ba_person_type as person_type,
            ba_usage_status as usage_status,
            account_name as client_name,
            segment,
            sales_name as sales_person,
            channel
        FROM cloud_analytics.acquisition_cube
        WHERE
            event = 'ba_created'
    ) as t1
    ON t0.billing_account_id = t1.billing_account_id
    '''

    exucute_query(query, hosts, cloud_clickhouse_param_dict)

    query = '''
    DROP TABLE IF EXISTS cloud_analytics.cloud_consumption_cube
    '''

    exucute_query(query, hosts, cloud_clickhouse_param_dict)

    query = '''
    RENAME TABLE cloud_analytics.cloud_consumption_cube_temp TO cloud_analytics.cloud_consumption_cube
    '''

    exucute_query(query, hosts, cloud_clickhouse_param_dict)

    query = '''
    DROP TABLE IF EXISTS cloud_analytics.cloud_consumption_cube_temp
    '''

    exucute_query(query, hosts, cloud_clickhouse_param_dict)

if __name__ == '__main__':
    main()
