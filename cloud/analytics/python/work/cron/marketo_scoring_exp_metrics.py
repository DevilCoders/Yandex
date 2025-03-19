#!/usr/bin/env python2
# -*- coding: utf-8 -*-
import pandas as pd, numpy as np, catboost, logging, os, sys, requests, datetime, time, pymysql
from sklearn.model_selection import train_test_split
from sklearn.utils import shuffle
from sklearn.metrics import confusion_matrix, recall_score, precision_score, roc_auc_score
import scipy.stats as stats
module_path = os.path.abspath(os.path.join('/home/ktereshin/yandex/arcadia/cloud/analytics/python/work'))
if module_path not in sys.path:
    sys.path.append(module_path)
module_path = os.path.abspath(os.path.join('/home/ktereshin/yandex/cron'))
if module_path not in sys.path:
    sys.path.append(module_path)

from data_loader import clickhouse
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
    stat_creds
)

def execute_query(query, cluster, alias, token, timeout=600):
    proxy = "http://{}.yt.yandex.net".format(cluster)
    s = requests.Session()
    url = "{proxy}/query?database={alias}&password={token}".format(proxy=proxy, alias=alias, token=token)
    resp = s.post(url, data=query, timeout=timeout)
    resp.raise_for_status()
    rows = resp.content.strip().split('\n')
    return rows

def chyt_execute_query(query, cluster, alias, token, columns):
    i = 0
    while True:
        try:
            result = execute_query(query=query, cluster=cluster, alias=alias, token=token)
            users = pd.DataFrame([row.split('\t') for row in result], columns = columns)
            return users
        except Exception as err:
            print(err)
            i += 1
            if i > 10:
                print('Break Excecution')
                break
def main():
    last_day = str(datetime.date.today() - datetime.timedelta(days = 8))
    cluster_yt = clusters.yt.Hahn(
        token = yt_creds['value']['token'],
        pool = yt_creds['value']['pool']
    )

    cluster = 'hahn'
    alias = "*cloud_analytics"
    token = '%s' % (yt_creds['value']['token'])

    query = '''
    SELECT
        t0.*,
        t1.is_fraud
    FROM(
        SELECT
            paid_prob,
            group,
            prob_cat,
            billing_account_id,
            score_cat,
            puid,
            score,
            first_trial_consumption_date,
            prob_threshold
        FROM "//home/cloud_analytics/scoring/reports/leads"
        WHERE
            first_trial_consumption_date >= '2019-04-12' AND first_trial_consumption_date <= '{0}'
    ) as t0
    ANY LEFT JOIN (
        SELECT
            billing_account_id,
            is_fraud
        FROM "//home/cloud_analytics/cubes/acquisition_cube/cube"
        WHERE
            event = 'ba_created'
    ) as t1
    ON t0.billing_account_id = t1.billing_account_id
    '''.format(last_day)
    columns = [
        'paid_prob',
        'group',
        'prob_cat',
        'billing_account_id',
        'score_cat',
        'puid',
        'score',
        'first_trial_consumption_date',
        'prob_threshold',
        'is_fraud'
    ]
    users = chyt_execute_query(query=query, cluster=cluster, alias=alias, token=token, columns = columns)

    int_cols = ['paid_prob', 'score', 'prob_threshold']
    for col in int_cols:
        users[col] = users[col].astype(float)

    cnx = pymysql.connect(
        user='cloud8',
        password=crm_sql_creds['value']['readonly'],
        host='c-mdb8t5pqa6cptk82ukmc.rw.db.yandex.net',
        port = 3306,
        database='cloud8'
    )

    query = '''
    SELECT
        sorce.parent_id lead_id,
        CONCAT('lead_changed_',CAST(sorce.field_name as CHAR(50))) as event,
        sorce.after_value_string lead_state,
        MIN(CAST(sorce.date_created as char)) as event_time,
        MIN(IFNULL(l1.account_name,'')) as account_name,
        MIN(IFNULL(l1.passport_uid,''))  as puid,
        MIN(IFNULL(l1.ba_id,'')) as billing_account_id,
        MIN(l1.lead_source) as lead_source,
        MIN(l1.lead_source_description) as lead_source_description,
        MIN(l2.email_address) as lead_email,
        MIN(l3.user_name) sales_name
    FROM
        leads_audit as sorce
    INNER JOIN  leads l1
        ON l1.id=sorce.parent_id AND l1.deleted = 0
    LEFT JOIN  email_addr_bean_rel l2_1
        ON l1.id=l2_1.bean_id AND l2_1.bean_module = 'Leads' AND l2_1.primary_address = 1
    LEFT JOIN  email_addresses l2
        ON l2.id=l2_1.email_address_id
    LEFT JOIN  users l3
        ON sorce.after_value_string=l3.id
    WHERE
        sorce.after_value_string != ''
        AND CONCAT('lead_changed_',CAST(sorce.field_name as CHAR(50))) = 'lead_changed_status'
        AND IFNULL(l1.ba_id,'') != ''
    GROUP BY
        sorce.parent_id,
        CONCAT('lead_changed_',CAST(sorce.field_name as CHAR(50))),
        sorce.after_value_string
    '''

    leads_history = pd.read_sql_query(query, cnx)
    cnx.close()

    leads_history_pivot = pd.pivot_table(
        leads_history,
        index = 'billing_account_id',
        columns = 'lead_state',
        values = 'lead_email',
        aggfunc = lambda x: 1,
        fill_value = 0
    ).reset_index()

    statuses = pd.merge(
        users[users['score']> 0.2],
        leads_history_pivot,
        on = 'billing_account_id',
        how = 'left'
    )
    statuses['Recycled'] = statuses['Recycled'].fillna(1)
    statuses = statuses.fillna(0)

    cluster_yt.write('//home/cloud_analytics/scoring/reports/exp_results/lead_statuses_', statuses)

    schema = {
        "Assigned": float,
        "Converted": float,
        "In Process": float,
        "New": float,
        "Pending": float,
        "Recycled": float,
        "billing_account_id": str,
        "first_trial_consumption_date": str,
        "group": str,
        "paid_prob": float,
        "prob_cat": str,
        "prob_threshold": float,
        "puid": str,
        "score": float,
        "score_cat": str,
        "is_fraud": str
    }
    try:
        cluster_yt.driver.remove('//home/cloud_analytics/scoring/reports/exp_results/lead_statuses')
    except:
        pass
    job = cluster_yt.job()
    job.table('//home/cloud_analytics/scoring/reports/exp_results/lead_statuses_') \
    .put('//home/cloud_analytics/scoring/reports/exp_results/lead_statuses', schema = schema)
    job.run()

    cnx = pymysql.connect(
        user='cloud8',
        password=crm_sql_creds['value']['readonly'],
        host='c-mdb8t5pqa6cptk82ukmc.rw.db.yandex.net',
        port = 3306,
        database='cloud8'
    )

    query = '''
    SELECT
        IFNULL(calls.id,'') call_id,
        CAST(calls.date_start AS char) event_time,
        IFNULL(calls.description,'') call_description,
        IFNULL(calls.parent_type,'') call_parent_type,
        IFNULL(l1.id,'') lead_id,
        IFNULL(l1.passport_uid,'') puid,
        IFNULL(t2.ba_id,'') billing_account_id,
        IFNULL(l1.phone_mobile,'') lead_phone,
        IFNULL(l2.email_address,'') lead_email,
        IFNULL(calls.name,'') call_name,
        IFNULL(l3.user_name,'') sales_name,
        'call' as event,
        'call' as lead_state,
        group_concat(CASE WHEN l4.name LIKE '%едозвон%' OR lower(calls.status) LIKE '%not_reached%' THEN 'unreachible' ELSE 'reachible' END) call_status,
        group_concat(IFNULL(l4.name,'no_tag')) call_tag
    FROM calls
    LEFT JOIN  calls_leads l1_1
        ON calls.id=l1_1.call_id
    LEFT JOIN  leads l1
        ON l1.id=l1_1.lead_id AND l1.deleted=0
    LEFT JOIN leads_billing_accounts as t1
        ON l1.id = t1.leads_id
    LEFT JOIN billingaccounts as t2
        ON t2.id = t1.billingaccounts_id
    LEFT JOIN  email_addr_bean_rel l2_1
        ON l1.id=l2_1.bean_id AND l2_1.bean_module = 'Leads' AND l2_1.primary_address = 1 AND l2_1.deleted=0
    LEFT JOIN  email_addresses l2
        ON l2.id=l2_1.email_address_id AND l2.deleted=0
    LEFT JOIN  users l3
        ON calls.assigned_user_id=l3.id AND l3.deleted=0
    LEFT JOIN  tag_bean_rel l4_1
        ON calls.id=l4_1.bean_id AND l4_1.bean_module = 'Calls' AND l4_1.deleted=0
    LEFT JOIN  tags l4 ON l4.id=l4_1.tag_id AND l4.deleted=0
    WHERE
        calls.status IN ('Held', 'not_reached')
    GROUP BY
        call_id,
        event_time,
        call_description,
        call_parent_type,
        lead_id,
        lead_source,
        lead_source_description,
        lead_phone,
        lead_email,
        call_name,
        sales_name,
        event,
        lead_state,
        puid,
        billing_account_id
    '''

    calls_df = pd.read_sql_query(query, cnx)
    cnx.close()

    calls_df['call_status'] = calls_df['call_status'].apply(lambda x: 'unreachible' if x =='unreachible' else 'reachible')

    calls = pd.pivot_table(
        calls_df,
        index = ['billing_account_id', 'call_id'],
        columns = 'call_status',
        values = 'lead_email',
        aggfunc = lambda x: 1,
        fill_value = 0
    ).reset_index()

    calls = pd.merge(
        calls,
        users[users['score']> 0.2],
        on = 'billing_account_id',
        how = 'inner'
    )

    cluster_yt.write('//home/cloud_analytics/scoring/reports/exp_results/calls_', calls)

    schema = {
        "unreachible": int,
        "reachible": int,
        "call_id": str,
        "billing_account_id": str,
        "first_trial_consumption_date": str,
        "group": str,
        "paid_prob": float,
        "prob_cat": str,
        "prob_threshold": float,
        "puid": str,
        "score": float,
        "score_cat": str,
        "is_fraud": str
    }
    try:
        cluster_yt.driver.remove('//home/cloud_analytics/scoring/reports/exp_results/calls')
    except:
        pass
    job = cluster_yt.job()
    job.table('//home/cloud_analytics/scoring/reports/exp_results/calls_') \
    .put('//home/cloud_analytics/scoring/reports/exp_results/calls', schema = schema)
    job.run()

if __name__ == '__main__':
    main()
