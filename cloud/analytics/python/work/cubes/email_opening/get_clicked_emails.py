#!/usr/bin/env python2
# -*- coding: utf-8 -*-
import pandas as pd, datetime, ast
from nile.api.v1 import (
    clusters,
    aggregators as na,
    extractors as ne,
    filters as nf,
    Record
)
from vault_client import instances
def get_datetime_from_epoch(epoch):
    try:
        return str(datetime.datetime.fromtimestamp(int(epoch)))
    except:
        return None

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

def get_mail_id(mail_desc):
    mail_desc = mail_desc.lower()
    if '|' in mail_desc:
        if '2019' in mail_desc.split('|')[0]:
            st = mail_desc.split('|')[1].lower()
            st = ' '.join(st.split())
            return st.replace(' ', '-')
        else:
            st = mail_desc.split('|')[0].lower()
            st = ' '.join(st.split())
            return st.replace(' ', '-')
    else:
        if 'test' in mail_desc or 'тест' in mail_desc:
            return 'testing'

        if 'terms-update' in mail_desc or 'terms_update' in mail_desc:
            return 'terms-update'

        if 'activation' in mail_desc or 'act-' in mail_desc:
            return 'ba-activation'

        if 'scenario' in mail_desc:
            return '3-scenarios-to-paid'

        if 'go-to-paid' in mail_desc and 'promo' not in mail_desc:
            return 'go-to-paid'

        if 'beginners' in mail_desc and 'promo' in mail_desc:
            return 'promo-beginners'

        if 'active-user' in mail_desc and 'promo' in mail_desc:
            return 'promo-active-users'

        if 'we-are-public' in mail_desc:
            return 'we-are-public'

        if 'start-usage' in mail_desc and 'promo' in mail_desc:
            return 'promo-start-usage'

        if 'go-to-paid' in mail_desc and 'promo' in mail_desc:
            return 'promo-go-to-paid'

        if 'reminder' in mail_desc and 'promo' in mail_desc:
            return 'promo-reminder'

        if 'trial' in mail_desc and 'extended' in mail_desc:
            return 'trial-extended'

        if 'typical' in mail_desc:
            return 'typical-task'

        if 'grant' in mail_desc and 'use' in mail_desc:
            return 'use-grant'

        if 'open' in mail_desc:
            return 'we-are-open'

        if 'follow' in mail_desc:
            return 'webinar-follow-up'

        if 'error' in mail_desc:
            return 'error-payment-method'

        if 'cloud-functionality' in mail_desc:
            return 'cloud-functionality'

        if '[' in mail_desc:
            st = mail_desc.split('[')[0].lower()
            st = ' '.join(st.split())
            if st[-1] == '-':
                return st[:-1].replace(' ', '-')
            return st.replace(' ', '-')
        return mail_desc.replace(' ', '-')

def works_with_emails(mail_):
    mail_parts = str(mail_).split('@')
    if len(mail_parts) > 1:
        if 'yandex.' in mail_parts[1].lower() or 'ya.' in mail_parts[1].lower():
            domain = 'yandex.ru'
            login = mail_parts[0].lower().replace('.', '-')
            return login + '@' + domain
        else:
            return mail_.lower()

def get_last_not_empty_table(folder_path, job):
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

def pivot_events(groups):
    for key, records in groups:
        event_list = [
            'cloud_created',
            'ba_created',
            'first_trial_consumption',
            'first_paid_consumption',
            'ba_became_paid'
        ]
        result_dict = {}
        baid = None
        puid = None
        for rec in records:
            if rec['billing_account_id']:
                baid = rec['billing_account_id']
            if rec['puid']:
                puid = rec['puid']
            if rec['event'] in event_list:
                result_dict[rec['event'] + '_time'] = rec['event_time']
                result_dict['is_' + rec['event']] = 1
                event_list.remove(rec['event'])
            else:
                continue
        result_dict['billing_account_id'] = baid
        result_dict['puid'] = puid
        yield Record(key, **result_dict)

def main():
    client = instances.Production()
    yt_creds = client.get_version('ver-01d33pgv8pzc7t99s3egm24x47')
    cluster = clusters.yt.Hahn(
        token = yt_creds['value']['token'],
        pool = yt_creds['value']['pool']
    )

    paths_dict_test = {
        'open_email': '//home/cloud_analytics/import/marketo/click_email',
        'open_email_1': '//home/cloud_analytics/import/marketo_bak_04.03.2019/click_email',
        'open_email_2': '//home/cloud_analytics/import/marketo_bak_06.03.2019/click_email',
        'open_email_3': '//home/cloud_analytics/import/marketo_bak_22.02.2019/click_email',
        'mail_open_event':'//home/cloud_analytics_test/cooking_cubes/acquisition_cube/sources/click_email',
        'cloud_owners':'//home/cloud_analytics/import/iam/cloud_owners/1h'
    }
    paths_dict_prod = {
        'open_email': '//home/cloud_analytics/import/marketo/click_email',
        'open_email_1': '//home/cloud_analytics/import/marketo_bak_04.03.2019/click_email',
        'open_email_2': '//home/cloud_analytics/import/marketo_bak_06.03.2019/click_email',
        'open_email_3': '//home/cloud_analytics/import/marketo_bak_22.02.2019/click_email',
        'mail_open_event':'//home/cloud_analytics/cooking_cubes/acquisition_cube/sources/click_email',
        'cloud_owners':'//home/cloud_analytics/import/iam/cloud_owners/1h'
    }

    mode = 'test'
    if mode == 'test':
        paths_dict = paths_dict_test
    elif mode == 'prod':
        paths_dict = paths_dict_prod

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
    path_cloud_owners = get_last_not_empty_table(paths_dict['cloud_owners'], job)

    job = cluster.job()
    open_email = job.concat(
            job.table(paths_dict_test['open_email']),
            job.table(paths_dict_test['open_email_1']),
            job.table(paths_dict_test['open_email_2']),
            job.table(paths_dict_test['open_email_3'])
        ) \
        .filter(
            nf.custom(lambda x: 'terms-update' not in get_mail_id(x), 'mailing_name')
        ) \
        .unique(
            'marketo_id'
        ) \
        .project(
            event_time = ne.custom(lambda x: str(get_datetime_from_epoch(x)), 'created'),
            email = ne.custom(works_with_emails,'email'),
            user_settings_email = ne.custom(works_with_emails,'user_settings_email'),
            event = ne.const('click_mail'),
            channel = ne.const('Emailing'),
            utm_source = ne.const('emailing'),
            utm_medium = ne.const('emailing'),
            utm_campaign = ne.custom(get_mail_id, 'mailing_name'),
            utm_content = 'mailing_name',
            utm_term = 'campaign_name'

        )
    clouds = job.table(path_cloud_owners) \
        .project(
            'login',
            email = ne.custom(lambda x,y: works_with_emails(x) if x == y else works_with_emails(x), 'email', 'user_settings_email'),
            puid = 'passport_uid'
        ) \
        .unique('email')

    res = open_email \
        .join(
            clouds,
            by = 'email',
            type = 'left'
        ) \
        .project(
        'event_time',
        'event',
        'puid',
            **apply_types_in_project(visits_settings)
        ) \
        .put(paths_dict['mail_open_event'])
    job.run()

if __name__ == '__main__':
    main()

