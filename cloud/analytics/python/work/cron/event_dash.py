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
    mode = 'test'
    bot = telebot.TeleBot(telebot_creds['value']['token'])
    cluster_yt = clusters.yt.Hahn(
        token = yt_creds['value']['token'],
        pool = yt_creds['value']['pool']
    )
    if mode == 'test':
        path = '//home/cloud_analytics_test/cooking_cubes/events/oppt'
    elif mode == 'prod':
        path = '//home/cloud_analytics/cooking_cubes/events/oppt'

    cnx = pymysql.connect(
        user='cloud8',
        password=crm_sql_creds['value']['readonly'],
        host='c-mdb8t5pqa6cptk82ukmc.rw.db.yandex.net',
        port = 3306,
        database='cloud8'
    )

    query = '''
    SELECT
        opportunities.oppty_id id,
        IFNULL(opportunities.name,'') name,
        IFNULL(opportunities.scenario,'') scenario,
        CAST(opportunities.date_entered AS char) event_time,
        'opportunity' as event,
        IFNULL(opportunities.description,'') description,
        IFNULL(opportunities.sales_status,'') sales_status,
        group_concat(IFNULL(l2.ba_id,'no_billing_account_id')) as billing_account_id,
        group_concat(IFNULL(l1.name,'no_dimensions')) as dimentions
    FROM opportunities
    LEFT JOIN dimensions_bean_rel l1_1
        ON opportunities.id=l1_1.bean_id AND l1_1.deleted=0
        AND l1_1.bean_module = 'Opportunities'
    LEFT JOIN  dimensions l1
        ON l1.id=l1_1.dimension_id
        AND l1.deleted=0
    LEFT JOIN  accounts_opportunities l2_1
        ON opportunities.id=l2_1.opportunity_id
        AND l2_1.deleted=0
    LEFT JOIN  billingaccounts l2
        ON l2.account_id=l2_1.account_id
        AND l2.deleted=0
    WHERE
         opportunities.deleted=0
    GROUP BY
        id,
        name,
        scenario,
        event_time,
        description,
        sales_status,
        event
    UNION ALL
    SELECT
        IFNULL(accounts.id,'') id,
        IFNULL(accounts.name,'') name,
        group_concat(IFNULL(l1.scenario,'')) scenario,
        CAST(accounts.date_entered AS char) event_time,
        'account' as event,
        IFNULL(accounts.description,'') description,
        group_concat(IFNULL(l1.sales_status,'')) sales_status,
        group_concat(IFNULL(l2.ba_id,'no_billing_account_id')) as billing_account_id,
        group_concat(IFNULL(l3.name,'no_dimensions')) as dimentions

    FROM accounts
    LEFT JOIN  accounts_opportunities l1_1
        ON accounts.id=l1_1.account_id AND l1_1.deleted=0
    LEFT JOIN  opportunities l1
        ON l1.id=l1_1.opportunity_id AND l1.deleted=0
    LEFT JOIN  billingaccounts l2
        ON accounts.id=l2.account_id AND l2.deleted=0
    LEFT JOIN  dimensions_bean_rel l3_1
        ON accounts.id=l3_1.bean_id AND l3_1.deleted=0
        AND l3_1.bean_module = 'Accounts'
    LEFT JOIN  dimensions l3
        ON l3.id=l3_1.dimension_id AND l3.deleted=0
    WHERE
        accounts.deleted=0
    GROUP BY
        id,
        name,
        event_time,
        description,
        event
    '''

    oppt = pd.read_sql_query(query, cnx)
    cnx.close()

    cluster_yt.write(path, oppt)

    schema = {
        "oppt_id": str,
        "oppt_name": str,
        "oppt_scenario": str,
        "event_time": str,
        "event": str,
        "oppt_description": str,
        "oppt_sales_status":str,
        "billing_account_id": str,
        "dimentions": str
    }
    job = cluster_yt.job()
    job.table(path) \
    .project(
        **apply_types_in_project(schema)
    ) \
    .put(path + '_schema', schema = schema)
    job.run()

if __name__ == '__main__':
    main()
