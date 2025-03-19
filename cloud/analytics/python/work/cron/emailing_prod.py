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

def works_with_emails(mail_):
    mail_parts = str(mail_).split('@')
    if len(mail_parts) > 1:
        if 'yandex.' in mail_parts[1].lower() or 'ya.' in mail_parts[1].lower():
            domain = 'yandex.ru'
            login = mail_parts[0].lower().replace('.', '-')
            return login + '@' + domain
        else:
            return mail_.lower()

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
        ba_state = None
        block_reason = None
        for rec in records:
            if rec['billing_account_id']:
                baid = rec['billing_account_id']
            if rec['puid']:
                puid = rec['puid']
            if rec['ba_state']:
                ba_state = rec['ba_state']
            if rec['block_reason']:
                block_reason = rec['block_reason']
            if rec['event'] in event_list:
                result_dict[rec['event'] + '_time'] = rec['event_time']
                result_dict['is_' + rec['event']] = 1
                event_list.remove(rec['event'])
            else:
                continue
        result_dict['billing_account_id'] = baid
        result_dict['puid'] = puid
        result_dict['ba_state'] = ba_state
        result_dict['block_reason'] = block_reason
        yield Record(key, **result_dict)

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

def main():
    cluster = clusters.yt.Hahn(
        token = yt_creds['value']['token'],
        pool = yt_creds['value']['pool']
    )

    paths_dict_test = {
        'send_email': '//home/cloud_analytics/import/marketo/send_email',
        'email_delivered': '//home/cloud_analytics/import/marketo/email_delivered',
        'open_email': '//home/cloud_analytics/import/marketo/open_email',
        'click_email': '//home/cloud_analytics/import/marketo/click_email',
        'acquisition_cube': '//home/cloud_analytics/cubes/acquisition_cube/cube',
        'emailing_cube':'//home/cloud_analytics_test/cubes/emailing/cube'
    }
    paths_dict_prod = {
        'send_email': '//home/cloud_analytics/import/marketo/send_email',
        'email_delivered': '//home/cloud_analytics/import/marketo/email_delivered',
        'open_email': '//home/cloud_analytics/import/marketo/open_email',
        'click_email': '//home/cloud_analytics/import/marketo/click_email',
        'acquisition_cube': '//home/cloud_analytics/cubes/acquisition_cube/cube',
        'emailing_cube':'//home/cloud_analytics/cubes/emailing/cube',
    }

    mode = 'test'
    if mode == 'test':
        paths_dict = paths_dict_test
    elif mode == 'prod':
        paths_dict = paths_dict_prod

    job = cluster.job()
    send_email = job.table(paths_dict_test['send_email']) \
        .unique(
            'marketo_id'
        ) \
        .project(
            event = ne.const('email_sended'),
            event_time = ne.custom(lambda x: get_datetime_from_epoch(x), 'created'),
            email = ne.custom(works_with_emails,'email'),
            mail_id = ne.custom(get_mail_id, 'mailing_name'),
            mailing_name = 'mailing_name'

        )



    email_delivered = job.table(paths_dict_test['email_delivered']) \
        .unique(
            'marketo_id'
        )\
        .project(
            delivery_time = ne.custom(lambda x: get_datetime_from_epoch(x), 'created'),
            email = ne.custom(works_with_emails,'email'),
            mail_id = ne.custom(get_mail_id, 'mailing_name'),
            mailing_name = 'mailing_name'

        ) \
        .groupby(
            'email',
            'mail_id',
            'mailing_name'
        ) \
        .aggregate(
            delivery_time = na.min('delivery_time')
        )

    open_email = job.table(paths_dict_test['open_email']) \
        .unique(
            'marketo_id'
        ) \
        .project(
            open_time = ne.custom(lambda x: get_datetime_from_epoch(x), 'created'),
            email = ne.custom(works_with_emails,'email'),
            mail_id = ne.custom(get_mail_id, 'mailing_name'),
            mailing_name = 'mailing_name'

        ) \
        .groupby(
            'email',
            'mail_id',
            'mailing_name'
        ) \
        .aggregate(
            open_time = na.min('open_time')
        )

    click_email = job.table(paths_dict_test['click_email']) \
        .unique(
            'marketo_id'
        ) \
        .project(
            click_time = ne.custom(lambda x: get_datetime_from_epoch(x), 'created'),
            email = ne.custom(works_with_emails,'email'),
            mail_id = ne.custom(get_mail_id, 'mailing_name'),
            mailing_name = 'mailing_name'

        ) \
        .groupby(
            'email',
            'mail_id',
            'mailing_name'
        ) \
        .aggregate(
            click_time = na.min('click_time')
        )

    funnel_events = job.table(paths_dict_test['acquisition_cube']) \
        .filter(
            nf.custom(lambda x: x not in ['visit', 'call', 'day_use'])
        ) \
        .project(
            'mail_tech',
            'mail_testing',
            'mail_info',
            'mail_feature',
            'mail_event',
            'mail_promo',
            'mail_billing',
            event = 'event',
            event_time = 'event_time',
            email = ne.custom(works_with_emails,'user_settings_email'),
            billing_account_id = 'billing_account_id',
            ba_state = 'ba_state',
            block_reason = 'block_reason',
            puid = 'puid',
        ) \
        .groupby(
            'email',
            'mail_tech',
            'mail_testing',
            'mail_info',
            'mail_feature',
            'mail_event',
            'mail_promo',
            'mail_billing'
        ) \
        .sort(
            'event_time'
        ) \
        .reduce(
            pivot_events
        )


    res = send_email \
        .join(
            email_delivered,
            by = ['email', 'mail_id', 'mailing_name'],
            type = 'left'
        ) \
        .join(
            open_email,
            by = ['email', 'mail_id', 'mailing_name'],
            type = 'left'
        ) \
        .join(
            click_email,
            by = ['email', 'mail_id', 'mailing_name'],
            type = 'left'
        ) \
        .join(
            funnel_events,
            by = ['email'],
            type = 'left'
        ) \
        .unique(
            "ba_became_paid_time",
            "ba_created_time",
            "billing_account_id",
            "ba_state",
            "block_reason",
            "click_time",
            "cloud_created_time",
            "delivery_time",
            "email",
            "event",
            "event_time",
            "first_paid_consumption_time",
            "first_trial_consumption_time",
            "is_ba_became_paid",
            "is_ba_created",
            "is_cloud_created",
            "is_first_paid_consumption",
            "is_first_trial_consumption",
            "mail_id",
            "open_time",
            "mailing_name"
        ) \
        .put(paths_dict_test['emailing_cube']+'_temp')
    job.run()

    try:
        cluster.driver.remove(paths_dict_test['emailing_cube'])
    except:
        pass
    schema = {
        "ba_became_paid_time": str,
        "ba_created_time": str,
        "billing_account_id": str,
        "ba_state": str,
        "block_reason": str,
        "click_time": str,
        "cloud_created_time": str,
        "delivery_time": str,
        "email": str,
        "event": str,
        "event_time": str,
        "first_paid_consumption_time": str,
        "first_trial_consumption_time": str,
        "is_ba_became_paid": int,
        "is_ba_created": int,
        "is_cloud_created": int,
        "is_first_paid_consumption": int,
        "is_first_trial_consumption": int,
        "mail_id": str,
        "mailing_name": str,
        "open_time": str,
        "puid": str,
        "paid_consumption": float,
        "is_paid_more_then_10_rur": int,
        "is_trial_more_then_10_rur": int,
        "trial_consumption": float

    }
    job = cluster.job()
    source = job.table(paths_dict_test['emailing_cube']+'_temp')
    cunsumption = job.table(paths_dict_test['acquisition_cube']) \
        .filter(
            nf.or_(
                nf.custom(lambda x: x > 0, 'real_consumption'),
                nf.custom(lambda x: x > 0, 'trial_consumption')
            )
        ) \
        .project(
            'real_consumption',
            'trial_consumption',
            con_time = 'event_time',
            billing_account_id = 'billing_account_id',
            is_paid_more_then_10_rur = ne.custom(lambda x: 1 if x > 10 else 0, 'real_consumption_cum'),
            is_trial_more_then_10_rur = ne.custom(lambda x: 1 if x > 10 else 0, 'trial_consumption_cum')
        )
    source = source \
        .join(
            cunsumption,
            by = 'billing_account_id',
            type = 'left'
        ) \
        .groupby(
            "ba_became_paid_time",
            "ba_created_time",
            "billing_account_id",
            "ba_state",
            "block_reason",
            "click_time",
            "cloud_created_time",
            "delivery_time",
            "email",
            "event",
            "event_time",
            "first_paid_consumption_time",
            "first_trial_consumption_time",
            "is_ba_became_paid",
            "is_ba_created",
            "is_cloud_created",
            "is_first_paid_consumption",
            "is_first_trial_consumption",
            "mail_id",
            "mailing_name",
            "open_time",
            "puid",
        ) \
        .aggregate(
            paid_consumption = na.sum('real_consumption', missing = 0),
            trial_consumption = na.sum('trial_consumption', missing = 0),
            is_paid_more_then_10_rur = na.max('is_paid_more_then_10_rur'),
            is_trial_more_then_10_rur = na.max('is_trial_more_then_10_rur')
        ) \
        .project(
            **apply_types_in_project(schema)
        ) \
        .put(paths_dict_test['emailing_cube'], schema = schema)
    job.run()

if __name__ == '__main__':
    main()
