#!/usr/bin/env python2
# -*- coding: utf-8 -*-
import os, sys, pandas as pd, vh, mysql.connector,click, datetime
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



client = instances.Production()
yt_creds = client.get_version('ver-01d33pgv8pzc7t99s3egm24x47')
crm_sql_creds = client.get_version('ver-01d3ktedjm6ptsvwf1xq161hwk')
cluster = clusters.yt.Hahn(
    token = yt_creds['value']['token'],
    pool = yt_creds['value']['pool']
).env(

    templates=dict(
        dates='{2018-09-01..2019-02-07}'
    )
)

cnx = mysql.connector.connect(user='readonly', password=crm_sql_creds['value']['readonly'],
                              host='percona.prod.ya-cloud-crm.stable.qloud-d.yandex.net',
                              port = 5678,
                              database='cloud8'
                             )

query = '''
SELECT
    IFNULL(calls.id,'') call_id,
    calls.date_entered calls_date_entered,
    calls.date_modified calls_date_modified,
    IFNULL(calls.direction,'') calls_direction,
    calls.duration_hours calls_duration_hours,
    IFNULL(calls.duration_minutes,'') calls_duration_minutes,
    IFNULL(calls.email_reminder_sent,0) calls_email_reminder_sent,
    calls.date_end calls_date_end,IFNULL(calls.parent_type,'') calls_parent_type,
    IFNULL(calls.acl_team_set_id,'') calls_acl_team_set_id,
    calls.date_start calls_date_start,
    IFNULL(calls.status,'') calls_status,
    IFNULL(l1.id,'') l1_id,
    IFNULL(l1.lead_source,'') l1_lead_source,IFNULL(l1.phone_mobile,'') l1_phone_mobile,
    IFNULL(l2.id,'') l2_id,
    IFNULL(l2.email_address,'') l2_email_address,
    IFNULL(calls.name,'') calls_name,
    IFNULL(l3.id,'') l3_id,
    IFNULL(l3.user_name,'') l3_user_name,
    IFNULL(calls.description,'') calls_description

FROM calls
LEFT JOIN  calls_leads l1_1 ON calls.id=l1_1.call_id AND l1_1.deleted=0

LEFT JOIN  leads l1 ON l1.id=l1_1.lead_id AND l1.deleted=0
LEFT JOIN  email_addr_bean_rel l2_1 ON l1.id=l2_1.bean_id AND l2_1.deleted=0
 AND l2_1.bean_module = 'Leads' AND l2_1.primary_address = 1
LEFT JOIN  email_addresses l2 ON l2.id=l2_1.email_address_id AND l2.deleted=0
LEFT JOIN  users l3 ON calls.assigned_user_id=l3.id AND l3.deleted=0
 WHERE ((1=1))
AND  calls.deleted=0
'''

calls_df = pd.read_sql_query(query, cnx)
cnx.close()

calls_df.rename(columns = lambda x: str(x).lower().replace(' ', '_').replace('"', ''), inplace = True)

for time_call in ['calls_date_entered', 'calls_date_modified', 'calls_date_end', 'calls_date_start']:
    calls_df[time_call] = calls_df[time_call].apply(lambda x: str(x))

cluster.write('//home/cloud_analytics/import/crm/calls_temp/calls_', calls_df)

visits_settings = {
    "ad_block": int,
    "age": str,
    "area": str,
    "channel": str,
    "channel_detailed": str,
    "city": str,
    "client_ip": str,
    "counter_id": str,
    "country": str,
    "device_model": str,
    "device_type": str,
    "duration": int,
    "first_visit_dt": str,
    "general_interests": str,
    "hits": int,
    "income": int,
    "interests": str,
    "is_bounce": int,
    "mobile_phone_vendor": int,
    "os": str,
    "page_views": int,
    "referer": str,
    "remote_ip":str,
    "resolution_depth": int,
    "resolution_height": int,
    "resolution_width": int,
    "search_phrase": str,
    "sex": str,
    "start_time": str,
    "total_visits": int,
    "user_id": str,
    "utm_campaign": str,
    "utm_content": str,
    "utm_medium": str,
    "utm_source": str,
    "utm_term": str,
    "visit_id": str,
    "visit_version": str,
    "window_client_height": int,
    "window_client_width": int,
    "is_robot": str,
    "start_url": str
}

job = cluster.job()
path_cloud_owners = get_last_not_empty_table('//home/cloud_analytics/import/iam/cloud_owners/1h')

job = cluster.job()
calls = job.table('//home/cloud_analytics/import/crm/calls_temp/calls_') \
    .project(
        email = ne.custom(works_with_emails, 'l2_email_address'),
        event = ne.const('call'),
        event_time = ne.custom(lambda x: str(datetime.datetime.strptime(str(x), '%Y-%m-%d %H:%M:%S') - datetime.timedelta(seconds = 10800)), 'calls_date_start'),
        channel = ne.const('Sales'),
        utm_source = ne.custom(lambda x: str(x).lower(), 'calls_parent_type'),
        utm_medium = ne.custom(lambda x: str(x).lower(), 'l1_lead_source'),
        utm_campaign = ne.custom(lambda x: str(x).lower(), 'l3_user_name'),
        utm_content = ne.custom(lambda x: str(x).lower(), 'calls_name'),
        call_detail = 'calls_description'
    )
clouds = job.table(path_cloud_owners) \
    .project(
        'login',
        email = ne.custom(works_with_emails, 'email'),
        puid = 'passport_uid'
    )
res = calls \
    .join(
        clouds,
        by = 'email',
        type = 'left'
    ) \
    .project(
    'event_time',
    'event',
    'puid',
    'call_detail',
        **apply_types_in_project(visits_settings)
    ) \
    .put('//home/cloud_analytics/import/crm/calls_temp/calls')
job.run()
