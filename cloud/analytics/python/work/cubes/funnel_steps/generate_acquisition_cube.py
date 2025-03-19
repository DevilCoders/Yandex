#!/usr/bin/env python2
# -*- coding: utf-8 -*-
import pandas as pd, datetime, ast, numpy as np, telebot, json, requests, os, sys, time
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

def get_time_delta(new_date, old_date):
    try:
        return (datetime.datetime.strptime(new_date, '%Y-%m-%d %H:%M:%S') - datetime.datetime.strptime(old_date, '%Y-%m-%d %H:%M:%S')).seconds

    except:
        pass

def apply_attr(result_dict_, last_visit_dict_):
    for col in last_visit_dict_:
        try:
            result_dict_[col] = last_visit_dict_[col]

        except:
            result_dict_[col ] = None

    result_dict_['session_start_time'] = last_visit_dict_['session_start_time']
    return result_dict_

def apply_attribution_reduce(groups):
    for key, records in groups:
        is_first_event = 1
        attr_window = 7776000
        utms = [
            "utm_campaign",
            "utm_content",
            "utm_medium",
            "utm_source",
            "utm_term",
            "channel"
        ]
        metrics = {
            'real_consumption': 0.0,
            'real_consumption_vat': 0.0,
            'real_payment': 0.0,
            'real_payment_vat': 0.0,
            'trial_consumption': 0.0,
            'trial_consumption_vat': 0.0
        }
        metric_sku = {}

        visits_settings = [
            "ad_block",
            "age",
            "area",
            "channel",
            "channel_detailed",
            "city",
            "client_ip",
            "counter_id",
            "country",
            "device_model",
            "device_type",
            "duration",
            "first_visit_dt",
            "general_interests",
            "hits",
            "income",
            "interests",
            "is_bounce",
            "mobile_phone_vendor",
            "os",
            "page_views",
            "referer",
            "remote_ip",
            "resolution_depth",
            "resolution_height",
            "resolution_width",
            "search_phrase",
            "sex",
            "start_time",
            "total_visits",
            "user_id",
            "utm_campaign",
            "utm_content",
            "utm_medium",
            "utm_source",
            "utm_term",
            "visit_id",
            "visit_version",
            "window_client_height",
            "window_client_width",
            "is_robot",
            "start_url"
        ]

        '''
        meta_data_dict = {
            "segment": None,
            "ba_currency": None,
            "ba_name": None,
            "ba_payment_cycle_type": None,
            "ba_person_type": None,
            "ba_state": None,
            "ba_type": None,
            "ba_usage_status": None,
            "balance_name": None,
            "billing_account_id": None,
            "login": None,
            "cloud_id": None,
            "cloud_name": None,
            "cloud_status": None,
            "mail_tech": None,
            "mail_testing": None,
            "mail_info": None,
            "mail_feature": None,
            "mail_event": None,
            "mail_promo": None,
            "mail_billing": None,
            "crm_client_name": None,
            "sales": None
        }
        '''

        last_visit_dict = {}
        last_direct_visit_dict = {}
        funnel_steps = {}
        record_list = []
        for rec in records:

            if not rec['event_time']:
                continue

            result_dict = rec.to_dict().copy()

            '''
            for meta in meta_data_dict:
                if meta in rec:
                    if rec[meta]:
                        meta_data_dict[meta] = rec[meta]
            '''

            if not rec['event'] in funnel_steps:
                funnel_steps[rec['event']] = rec['event_time']

            for metric in metrics:
                if metric in result_dict:
                    result_dict[metric] = convert_metric_to_float(result_dict[metric])
                else:
                    result_dict[metric] = 0.0

                if 'name' in result_dict and metric in result_dict:
                    if result_dict['name'] + '_' + metric in metric_sku:
                        metric_sku[result_dict['name'] + '_' + metric] += result_dict[metric]
                    else:
                         metric_sku[result_dict['name'] + '_' + metric] = 0.0
                #metrics[metric] += result_dict[metric]
                try:
                    result_dict[metric + '_cum'] = metric_sku[result_dict['name'] + '_' + metric]
                except:
                    result_dict[metric + '_cum'] = 0.0


            if is_first_event == 1:

                is_first_event = 0

                if rec['event'] not in ['visit', 'day_use', 'call', 'click_mail']:

                    for utm in utms:
                        result_dict[utm] = 'Unknown'

                    #yield Record(key, **result_dict)
                    record_list.append(result_dict)

                else:
                    if rec['event'] in ('visit', 'call', 'click_mail'):

                        if 'cloud.yandex' in str(rec['referer']):
                            continue

                        for visit_col in visits_settings:
                            last_visit_dict[visit_col] = rec[visit_col]

                        last_visit_dict['session_start_time'] = rec['event_time']

                        if (last_visit_dict['referer'] not in ['', None] and 'cloud.' not in last_visit_dict['referer'] and 'Organic' not in last_visit_dict['channel']) or rec['event'] in ['call', 'click_mail']:

                            for visit_col in visits_settings:
                                last_direct_visit_dict[visit_col] = rec[visit_col]

                            last_direct_visit_dict['session_start_time'] = rec['event_time']

                    #yield Record(key, **result_dict)
                    record_list.append(result_dict)

            else:

                if rec['event'] not in ['visit', 'day_use', 'call', 'click_mail']:

                    if last_direct_visit_dict or last_visit_dict:

                        if last_direct_visit_dict:
                            if get_time_delta(result_dict['event_time'], last_direct_visit_dict['session_start_time']) <= attr_window:
                                result_dict = apply_attr(result_dict, last_direct_visit_dict)

                        elif last_visit_dict:
                            if get_time_delta(result_dict['event_time'], last_visit_dict['session_start_time']) <= attr_window:
                                result_dict = apply_attr(result_dict, last_visit_dict)
                            else:
                                for utm in utms:
                                    result_dict[utm] = 'Unknown'

                    else:
                        for utm in utms:
                            result_dict[utm] = 'Unknown'

                    #yield Record(key, **result_dict)
                    record_list.append(result_dict)

                else:

                    if rec['event'] in ('visit', 'call', 'click_mail'):
                        if 'cloud.yandex' in str(rec['referer']):
                            continue

                        for visit_col in visits_settings:
                            last_visit_dict[visit_col] = rec[visit_col]

                        last_visit_dict['session_start_time'] = rec['event_time']

                        if (last_visit_dict['referer'] not in ['', None] and 'cloud.' not in last_visit_dict['referer'] and  'Organic' not in last_visit_dict['channel']) or rec['event'] in ['call', 'click_mail']:

                            for visit_col in visits_settings:
                                last_direct_visit_dict[visit_col] = rec[visit_col]

                            last_direct_visit_dict['session_start_time'] = rec['event_time']

                    #yield Record(key, **result_dict)
                    record_list.append(result_dict)

        for rec_dict in record_list:
            for event in funnel_steps:
                rec_dict['first_' + event + '_datetime'] = funnel_steps[event]
            #for meta in meta_data_dict:
                #rec_dict[meta] = meta_data_dict[meta]

            yield Record(key, **rec_dict)

def wait_for_done_running_proccess(os, file_names):
    #lst = lst.split('\n')
    proccess_running = 0
    files_done_dict = {}
    file_names = file_names.split(',')
    while True:
        for file_name in file_names:
            lst = os.popen('ps -ef | grep python').read()
            if file_name in lst:
                print('%s is running' % (file_name))
                time.sleep(30)
            else:
                files_done_dict[file_name] = 1
                proccess_running = 0
                print('%s Done' % (file_name))

        if len(files_done_dict) == len(file_names):
            break

def main():

    files_list = 'visits/get_source_visit_data.py,cubes/calls/get_calls.py,cubes/email_opening/get_clicked_emails.py,cubes/funnel_steps/get_ba_hist.py'
    wait_for_done_running_proccess(os, files_list)

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
    for path in ['billing_grants','cloud_owner_path', 'billing_accounts_path', 'sku_path', 'service_dict_path', 'billing_accounts_path', 'billing_records_path','transactions_path', 'billing_accounts_history_path']:
        valid_path = get_last_not_empty_table(paths_dict_temp[path], job)
        paths_dict[path] = valid_path

    job = cluster.job()


    #cloud created event
    clouds = job.table(paths_dict['cloud_owner_path']) \
        .project(
            'email',
            'first_name',
            'last_name',
            'login',
            'phone',
            'user_settings_email',
            'cloud_status',
            'cloud_name',
            mail_tech = ne.custom(lambda x: 1 if x else 0, 'mail_tech'),
            mail_testing = ne.custom(lambda x: 1 if x else 0, 'mail_testing'),
            mail_info = ne.custom(lambda x: 1 if x else 0, 'mail_info'),
            mail_feature = ne.custom(lambda x: 1 if x else 0, 'mail_tech'),
            mail_event = ne.custom(lambda x: 1 if x else 0, 'mail_event'),
            mail_promo = ne.custom(lambda x: 1 if x else 0, 'mail_promo'),
            mail_billing = ne.custom(lambda x: 1 if x else 0, 'mail_billing'),
            cloud_id = 'id',
            event = ne.const('cloud_created'),
            event_time = ne.custom(lambda x: ' '.join(str(x).split('+')[0].split('T')), 'cloud_created_at'),
            puid = 'passport_uid'

        ) \
        .groupby(
            'puid'
        ) \
        .aggregate(
            email = na.first('email', by='event_time'),
            first_name = na.first('first_name', by='event_time'),
            last_name = na.first('last_name', by='event_time'),
            login = na.first('login', by='event_time'),
            phone = na.first('phone', by='event_time'),
            user_settings_email = na.first('user_settings_email', by='event_time'),
            cloud_status = na.first('cloud_status', by='event_time'),
            cloud_name = na.first('cloud_name', by='event_time'),
            event = na.first('event', by='cloud_created_at'),
            event_time = na.first('event_time', by='cloud_created_at'),
            cloud_id = na.first('cloud_id', by='cloud_created_at'),
            mail_tech = na.first('mail_tech', by='cloud_created_at'),
            mail_testing = na.first('mail_testing', by='cloud_created_at'),
            mail_info = na.first('mail_info', by='cloud_created_at'),
            mail_feature = na.first('mail_feature', by='cloud_created_at'),
            mail_event = na.first('mail_event', by='cloud_created_at'),
            mail_promo = na.first('mail_promo', by='cloud_created_at'),
            mail_billing = na.first('mail_billing', by='cloud_created_at')
        )
    clouds_ = clouds \
        .unique(
            'puid'
        ) \
        .project(
            'email',
            'first_name',
            'last_name',
            'login',
            'phone',
            'user_settings_email',
            'cloud_status',
            'cloud_name',
            'cloud_id',
            'puid',
            'mail_tech',
            'mail_testing',
            'mail_info',
            'mail_feature',
            'mail_event',
            'mail_promo',
            'mail_billing'
        )

    offers = job.table(paths_dict['offers_path']) \
        .filter(
            nf.custom(lambda x: x not in [None, ''], 'passport_uid')
        ) \
        .unique(
            'passport_uid'
        ) \
        .project(
            promocode_client_name = 'client_name',
            promocode_type = 'type',
            puid = 'passport_uid',
            promocode_source = 'client_source',
            promocode_client_type = ne.custom(lambda x: 'Enterprise/ISV' if x == 'direct_offer' else 'Direct')
        )

    segments = job.table(paths_dict['client_segments']) \
        .unique(
            'billing_account_id'
        )

    passports = job.table(paths_dict['balance']) \
        .unique(
            'PASSPORT_ID'
        ) \
        .project(
            puid = 'PASSPORT_ID',
            balance_name = 'NAME'
        )

    ba_puid_dict_ = job.table(paths_dict['billing_accounts_path']) \
        .unique(
            'id'
        ) \
        .project(
            ba_created_at = 'created_at',
            ba_name = 'name',
            ba_state = 'state',
            ba_person_type = 'person_type',
            ba_payment_cycle_type = 'payment_cycle_type',
            ba_usage_status =  'usage_status',
            ba_currency = 'currency',
            ba_type = 'type',
            billing_account_id = 'id',
            puid = 'owner_id',
            block_reason = ne.custom(lambda x,y:get_reason(y) if x == 'suspended' else 'Unlocked', 'state', 'metadata')
        ) \
        .join(
            offers,
            by = 'puid',
            type = 'left'
        ) \
        .join(
            segments,
            by = 'billing_account_id',
            type = 'left'
        ) \
        .join(
            passports,
            by = 'puid',
            type = 'left'
        )

    ba_puid_dict_ = ba_puid_dict_ \
        .project(
            ne.all(),
            segment = ne.custom(lambda x: x.lower() if x not in ['', None] else 'mass', 'segment')
        )


    ba_puid_dict_hist = job.table(paths_dict['ba_hist']) \
        .unique(
            'billing_account_id',
            'date'
        ) \
        .project(
            date = 'date',
            ba_name = 'name',
            ba_state = 'state',
            ba_person_type = 'person_type',
            ba_payment_cycle_type = 'payment_cycle_type',
            ba_usage_status =  'usage_status',
            ba_currency = 'currency',
            ba_type = 'type',
            billing_account_id = 'billing_account_id',
            puid = 'owner_id',
            block_reason = 'block_reason'
        ) \
        .join(
            offers,
            by = 'puid',
            type = 'left'
        ) \
        .join(
            segments,
            by = 'billing_account_id',
            type = 'left'
        ) \
        .join(
            passports,
            by = 'puid',
            type = 'left'
        )

    ba_puid_dict_hist = ba_puid_dict_hist \
    .project(
        ne.all(),
        segment = ne.custom(lambda x: x.lower() if x not in ['', None] else 'mass', 'segment')
    ) \
    .unique('billing_account_id', 'date')



    sku_dict = job.table(paths_dict['sku_path']) \
        .filter(
            nf.custom(lambda x: x not in ['', None], 'id')
        ) \
        .unique(
            'id'
        ) \
        .project(
            'name',
            'service_id',
            sku_id = 'id'

        )

    service_dict = job.table(paths_dict['service_dict_path']) \
        .filter(
            nf.custom(lambda x: x not in ['', None], 'id')
        ) \
        .unique(
            'id'
        ) \
        .project(
            service_name = 'name',
            service_description = 'description',
            service_id = 'id'

        )


    sku_dict = sku_dict \
        .join(
            service_dict,
            by = 'service_id',
            type = 'left'
        )

    ba_puid_dict = ba_puid_dict_ \
        .join(
            clouds_,
            by = 'puid',
            type = 'left'
        )

    bas = ba_puid_dict \
        .unique(
            'billing_account_id'
        ) \
        .project(
            ne.all(),
            event = ne.const('ba_created'),
            event_time = ne.custom(get_datetime_from_epoch, 'ba_created_at')
        )

    #========================================
    ba_puid_dict__ = ba_puid_dict_.unique('puid')
    visits = job.table(paths_dict['visits_path'])
    calls = job.table(paths_dict['calls'])
    click_mail = job.table(paths_dict['click_email'])
    #open_mail = job.table(paths_dict['open_mail'])

    puid_sets = job.concat(
        clouds,
        visits,
        calls,
        click_mail
        ) \
        .join(
            ba_puid_dict__,
            by = 'puid',
            type = 'left'
        ) \
        .project(
            ne.all(),
            ba_name = ne.custom(lambda x: x if x else 'ba_not_created', 'ba_name'),
            ba_payment_cycle_type = ne.custom(lambda x: x if x else 'ba_not_created', 'ba_payment_cycle_type'),
            ba_person_type = ne.custom(lambda x: x if x else 'ba_not_created', 'ba_person_type'),
            ba_state = ne.custom(lambda x: x if x else 'ba_not_created', 'ba_state'),
            ba_type = ne.custom(lambda x: x if x else 'ba_not_created', 'ba_type'),
            ba_usage_status = ne.custom(lambda x: x if x else 'ba_not_created', 'ba_usage_status'),
            block_reason = ne.custom(lambda x: x if x else 'ba_not_created', 'block_reason'),
            segment = ne.custom(lambda x: x if x else 'unknown', 'segment'),
            promocode_client_name = ne.custom(lambda x: x if x else 'unknown', 'promocode_client_name'),
            promocode_type = ne.custom(lambda x: x if x else 'unknown', 'promocode_type'),
            promocode_source = ne.custom(lambda x: x if x else 'unknown', 'promocode_source'),
            promocode_client_type = ne.custom(lambda x: x if x else 'unknown', 'promocode_client_type'),
            balance_name = ne.custom(lambda x: x if x else 'unknown', 'balance_name'),
            sales = ne.custom(lambda x: x if x else 'unknown', 'sales'),
            crm_client_name = ne.custom(lambda x: x if x else 'unknown', 'crm_client_name')
        )

    #========================================
    ba_became_paid = job.table(paths_dict['billing_accounts_history_path']) \
        .filter(
            nf.custom(lambda x: str(x).lower() == 'paid', 'usage_status')
        ) \
        .groupby(
            'billing_account_id'
        ) \
        .aggregate(
            event_time = na.min('updated_at')
        ) \
        .project(
            'billing_account_id',
            event_time = ne.custom( get_datetime_from_epoch,'event_time'),
            event = ne.const('ba_became_paid')
        )

    first_trial_consumption = job.table(paths_dict['billing_records_path']) \
        .filter(
            nf.custom(lambda x: float(x) < 0, 'credit')
        ) \
        .groupby(
            'billing_account_id'
        ) \
        .aggregate(
            event_time = na.first('date', by='date'),
            sku_id = na.first('sku_id', by='date'),
            cloud_id = na.first('cloud_id', by='date')
        ) \
        .project(
            'billing_account_id',
            'sku_id',
            'cloud_id',
            event_time = ne.custom( lambda x:  str(x) + ' 23:59:57','event_time'),
            event = ne.const('first_trial_consumption')
        )


    first_paid_consumption = job.table(paths_dict['billing_records_path']) \
        .filter(
            nf.custom(lambda x, y: float(x) + float(y) > 0, 'cost', 'credit')
        ) \
        .groupby(
            'billing_account_id'
        ) \
        .aggregate(
            event_time = na.first('date', by='date'),
            sku_id = na.first('sku_id', by='date'),
            cloud_id = na.first('cloud_id', by='date'),
        ) \
        .project(
            'billing_account_id',
            'sku_id',
            'cloud_id',
            event_time = ne.custom( lambda x:  str(x) + ' 23:59:58','event_time'),
            event = ne.const('first_paid_consumption')
        )

    first_payment = job.table(paths_dict['transactions_path']) \
        .filter(
            nf.custom(lambda x: x == 'payments', 'type'),
            nf.custom(lambda x: x == 'ok', 'status'),
        ) \
        .project(
            ne.all(),
            payment_type = ne.custom(get_payment_type, 'context')
        ) \
        .groupby(
            'billing_account_id'
        ) \
        .aggregate(
            event_time = na.first('modified_at', by='modified_at'),
            amount = na.first('amount', by='modified_at'),
            currency = na.first('currency', by='modified_at'),
            payment_type = na.first('payment_type', by='modified_at')
        ) \
        .project(
            'billing_account_id',
            'amount',
            'currency',
            'payment_type',
            event_time = ne.custom( get_datetime_from_epoch,'event_time'),
            event = ne.const('first_payment')
        )

    day_use_consumption = job.table(paths_dict['billing_records_path']) \
        .project(
            'billing_account_id',
            'sku_id',
            trial_consumption = ne.custom(lambda x: convert_metric_to_float(x)*-1, 'credit'),
            trial_consumption_vat = ne.custom(lambda x,y: convert_metric_to_float(x)*-1/1.18 if y < '2019-01-01' else convert_metric_to_float(x)*-1/1.2, 'credit', 'date'),
            real_consumption = ne.custom(lambda x, y: convert_metric_to_float(x) + convert_metric_to_float(y) if x not in [None, ''] and y not in [None, ''] else 0.0,'cost', 'credit'),
            real_consumption_vat = ne.custom(lambda x, y, z: (convert_metric_to_float(x) + convert_metric_to_float(y))/1.18 if z < '2019-01-01' else (convert_metric_to_float(x) + convert_metric_to_float(y))/1.2,'cost', 'credit', 'date'),
            event_time = ne.custom(lambda x: str(x) + ' 23:59:59', 'date'),
            date = 'date',
            event = ne.const('day_use'),
            real_payment = ne.const(0),
            real_payment_vat = ne.const(0)
        ) \
        .groupby(
            'billing_account_id',
            'sku_id',
            'event',
            'event_time',
            'date'
        ) \
        .aggregate(
            trial_consumption = na.sum('trial_consumption'),
            trial_consumption_vat = na.sum('trial_consumption_vat'),
            real_consumption = na.sum('real_consumption'),
            real_consumption_vat = na.sum('real_consumption_vat'),
            real_payment = na.sum('real_payment'),
            real_payment_vat = na.sum('real_payment_vat')
        )

    day_use_payments = job.table(paths_dict['transactions_path']) \
        .filter(
            nf.custom(lambda x: x == 'payments', 'type'),
            nf.custom(lambda x: x == 'ok', 'status'),
        ) \
        .project(
            'billing_account_id',
            'currency',
            sku_id = ne.const(''),
            event_time = ne.custom(convert_epoch_to_end_day, 'modified_at'),
            real_payment = ne.custom(lambda x: convert_metric_to_float(x), 'amount'),
            real_payment_vat = ne.custom(lambda x, y: convert_metric_to_float(x)/1.18 if convert_epoch_to_end_day(y).split(' ')[0] < '2019-01-01' else convert_metric_to_float(x)/1.2, 'amount', 'modified_at'),
            event = ne.const('day_use'),
            date = ne.custom(lambda x: convert_epoch_to_end_day(x).split(' ')[0], 'modified_at'),
            trial_consumption = ne.const(0),
            trial_consumption_vat = ne.const(0),
            real_consumption = ne.const(0),
            real_consumption_vat = ne.const(0)
        ) \
        .groupby(
            'billing_account_id',
            'event_time',
            'event',
            'sku_id',
            'date'
        ) \
        .aggregate(
            trial_consumption = na.sum('trial_consumption'),
            trial_consumption_vat = na.sum('trial_consumption_vat'),
            real_consumption = na.sum('real_consumption'),
            real_consumption_vat = na.sum('real_consumption_vat'),
            real_payment = na.sum('real_payment'),
            real_payment_vat = na.sum('real_payment_vat')
        )

    ba_sets_hist = job.concat(
        day_use_consumption,
        day_use_payments,
        ) \
        .join(
            ba_puid_dict_hist,
            by = ['billing_account_id', 'date'],
            type = 'left'
        ) \
        .join(
            sku_dict,
            by = 'sku_id',
            type = 'left'
        )

    ba_sets = job.concat(
        bas,
        first_payment,
        first_paid_consumption,
        first_trial_consumption,
        ba_became_paid
        ) \
        .join(
            ba_puid_dict_,
            by = 'billing_account_id',
            type = 'left'
        ) \
        .join(
            sku_dict,
            by = 'sku_id',
            type = 'left'
        )

    result = job.concat(
            puid_sets,
            ba_sets,
            ba_sets_hist
        )

    result.put('%s/%s_temp' % (paths_dict['cube_tmp'],str(datetime.datetime.today().date())))
    job.run()

    schema = {
        "ad_block": int,
        "age": str,
        "amount": str,
        "area": str,
        "ba_currency": str,
        "ba_name": str,
        "ba_payment_cycle_type": str,
        "ba_person_type": str,
        "ba_state": str,
        "ba_type": str,
        "ba_usage_status": str,
        "balance_name": str,
        "billing_account_id": str,
        "channel": str,
        "channel_detailed": str,
        "city": str,
        "client_ip": str,
        "cloud_id": str,
        "cloud_name": str,
        "cloud_status": str,
        "cost": float,
        "counter_id": str,
        "country": str,
        "credit": float,
        "currency": str,
        "device_model": str,
        "device_type": str,
        "duration": int,
        "email": str,
        "event": str,
        "event_time": str,
        "first_ba_became_paid_datetime": str,
        "first_ba_created_datetime": str,
        "first_cloud_created_datetime": str,
        "first_day_use_datetime": str,
        "first_first_paid_consumption_datetime": str,
        "first_first_payment_datetime": str,
        "first_first_trial_consumption_datetime": str,
        "first_name": str,
        "first_visit_datetime": str,
        "first_visit_dt": str,
        "general_interests": str,
        "hits": int,
        "income": int,
        "interests": str,
        "is_bounce": int,
        "is_robot": str,
        "last_name": str,
        "login": str,
        "mobile_phone_vendor": int,
        "name": str,
        "os": str,
        "page_views": int,
        "payment_type": str,
        "phone": str,
        "promocode_client_name": str,
        "promocode_client_type": str,
        "promocode_source": str,
        "promocode_type": str,
        "puid": str,
        "real_consumption": float,
        "real_consumption_cum": float,
        "real_payment": float,
        "real_payment_cum": float,
        "referer": str,
        "remote_ip": str,
        "resolution_depth": int,
        "resolution_height": int,
        "resolution_width": int,
        "search_phrase": str,
        "segment": str,
        "service_description": str,
        "service_id": str,
        "service_name": str,
        "session_start_time": str,
        "sex": str,
        "sku_id": str,
        "start_time": str,
        "start_url": str,
        "total_visits": int,
        "trial_consumption": float,
        "trial_consumption_cum": float,
        "user_id": str,
        "user_settings_email": str,
        "utm_campaign": str,
        "utm_content": str,
        "utm_medium": str,
        "utm_source": str,
        "utm_term": str,
        "visit_id": str,
        "visit_version": str,
        "window_client_height": int,
        "window_client_width": int,
        "mail_tech": int,
        "mail_testing": int,
        "mail_info": int,
        "mail_feature": int,
        "mail_event": int,
        "mail_promo": int,
        "mail_billing": int,
        "crm_client_name": str,
        "sales": str,
        "block_reason": str,
        "trial_consumption_vat": float,
        "trial_consumption_vat_cum": float,
        "real_consumption_vat": float,
        "real_consumption_vat_cum": float,
        "real_payment_vat": float,
        "real_payment_vat_cum": float,
    }

    job = cluster.job()
    cube = job.table('%s/%s_temp' % (paths_dict['cube_tmp'],str(datetime.datetime.today().date()))) \
        .groupby(
            'puid'
        ) \
        .sort(
            'event_time'
        ) \
        .reduce(
            apply_attribution_reduce,
            memory_limit = 2048
        ) \
        .project(
            **apply_types_in_project(schema)
        )
    cube \
    .put('%s/%s_to_validate' % (paths_dict['funnel_cube'],str(datetime.datetime.today().date())), schema = schema)
    #cube \
    #.put('%s/%s' % (paths_dict['funnel_cube'],'cube'), schema = schema)
    job.run()

    job = cluster.job()
    to_val = job.table('%s/%s_to_validate' % (paths_dict['funnel_cube'],str(datetime.datetime.today().date()))) \
        .filter(
            nf.custom(lambda x: x < str(datetime.date.today() - datetime.timedelta(days = 2)), 'event_time')
        ) \
        .project(
            'real_consumption',
            'trial_consumption',
            date = ne.custom(lambda x: str(x).split(' ')[0], 'event_time'),
            call = ne.custom(lambda x: 1 if str(x) == 'call' else 0, 'event'),
            visit = ne.custom(lambda x: 1 if str(x) == 'visit' else 0, 'event'),
            ba_created = ne.custom(lambda x: 1 if str(x) == 'ba_created' else 0, 'event'),
            first_paid_consumption = ne.custom(lambda x: 1 if str(x) == 'first_paid_consumption' else 0, 'event'),
            cloud_created = ne.custom(lambda x: 1 if str(x) == 'cloud_created' else 0, 'event'),
            first_trial_consumption = ne.custom(lambda x: 1 if str(x) == 'first_trial_consumption' else 0, 'event'),
            ba_became_paid = ne.custom(lambda x: 1 if str(x) == 'ba_became_paid' else 0, 'event'),
            day_use = ne.custom(lambda x: 1 if str(x) == 'day_use' else 0, 'event'),
            first_payment = ne.custom(lambda x: 1 if str(x) == 'first_payment' else 0, 'event')
        ) \
        .groupby(
            'date'
        ) \
        .aggregate(
            real_consumption_to_validate = na.sum('real_consumption', missing=0),
            trial_consumption_to_validate = na.sum('trial_consumption', missing=0),
            call_to_validate = na.sum('call', missing=0),
            visit_to_validate = na.sum('visit', missing=0),
            ba_created_to_validate = na.sum('ba_created', missing=0),
            first_paid_consumption_to_validate = na.sum('first_paid_consumption', missing=0),
            cloud_created_to_validate = na.sum('cloud_created', missing=0),
            first_trial_consumption_to_validate = na.sum('first_trial_consumption', missing=0),
            ba_became_paid_to_validate = na.sum('ba_became_paid', missing=0),
            day_use_to_validate = na.sum('day_use', missing=0),
            first_payment_to_validate = na.sum('first_payment', missing=0)
        )

    control = job.table('%s/%s' % (paths_dict['funnel_cube'],str(datetime.date.today() - datetime.timedelta(days = 1)))) \
        .filter(
            nf.custom(lambda x: x < str(datetime.date.today() - datetime.timedelta(days = 2)), 'event_time')
        ) \
        .project(
            'real_consumption',
            'trial_consumption',
            date = ne.custom(lambda x: str(x).split(' ')[0], 'event_time'),
            call = ne.custom(lambda x: 1 if str(x) == 'call' else 0, 'event'),
            visit = ne.custom(lambda x: 1 if str(x) == 'visit' else 0, 'event'),
            ba_created = ne.custom(lambda x: 1 if str(x) == 'ba_created' else 0, 'event'),
            first_paid_consumption = ne.custom(lambda x: 1 if str(x) == 'first_paid_consumption' else 0, 'event'),
            cloud_created = ne.custom(lambda x: 1 if str(x) == 'cloud_created' else 0, 'event'),
            first_trial_consumption = ne.custom(lambda x: 1 if str(x) == 'first_trial_consumption' else 0, 'event'),
            ba_became_paid = ne.custom(lambda x: 1 if str(x) == 'ba_became_paid' else 0, 'event'),
            day_use = ne.custom(lambda x: 1 if str(x) == 'day_use' else 0, 'event'),
            first_payment = ne.custom(lambda x: 1 if str(x) == 'first_payment' else 0, 'event')
        ) \
        .groupby(
            'date'
        ) \
        .aggregate(
            real_consumption = na.sum('real_consumption', missing=0),
            trial_consumption = na.sum('trial_consumption', missing=0),
            call = na.sum('call', missing=0),
            visit = na.sum('visit', missing=0),
            ba_created = na.sum('ba_created', missing=0),
            first_paid_consumption = na.sum('first_paid_consumption', missing=0),
            cloud_created = na.sum('cloud_created', missing=0),
            first_trial_consumption = na.sum('first_trial_consumption', missing=0),
            ba_became_paid = na.sum('ba_became_paid', missing=0),
            day_use = na.sum('day_use', missing=0),
            first_payment = na.sum('first_payment', missing=0)
        )
    to_val.join(
            control,
            by = 'date',
            type = 'full'
        ) \
        .sort('date') \
        .put('%s/validation' % (paths_dict['cube_tmp']))
    job.run()

    val_df = cluster.read('%s/validation' % (paths_dict['cube_tmp'])).as_dataframe().sort_values(by = 'date')

    col_list = ['ba_became_paid',
     'ba_created',
     'call',
     'cloud_created',
     'day_use',
     'first_paid_consumption',
     'first_payment',
     'first_trial_consumption',
     'real_consumption',
     'trial_consumption',
     'visit']

    diff_col = {}
    for col in col_list:
        val_df[col + '_diff'] = val_df[col] - val_df[col + '_to_validate']
        if val_df[col + '_diff'].sum() >= 1 or val_df[col + '_diff'].sum() <= -1:
            #print(col, val_df[col + '_diff'].sum())
            diff_col[col] = val_df[col + '_diff'].sum()

    ok = 0
    if ('visit' in diff_col or 'call' in diff_col or 'day_use' in diff_col):
        ok = 1
    if 'real_consumption' in diff_col:
        if abs(diff_col['real_consumption']) < 500:
            ok = 1
    if 'trial_consumption' in diff_col:
        if abs(diff_col['trial_consumption']) < 500:
            ok = 1

    if not diff_col:
        ok = 1

    if ok == 1:
        try:
            cluster.driver.remove('%s/%s' % (paths_dict['funnel_cube'],'cube'))
        except:
            pass
        try:
            cluster.driver.remove('%s/%s' % (paths_dict['funnel_cube'],str(datetime.datetime.today().date())))
        except:
            pass
        job = cluster.job()
        to_val = job.table('%s/%s_to_validate' % (paths_dict['funnel_cube'],str(datetime.datetime.today().date())))
        to_val.put('%s/%s' % (paths_dict['funnel_cube'],str(datetime.datetime.today().date())))
        to_val.put('%s/%s' % (paths_dict['funnel_cube'],'cube'))
        job.run()

        #cluster.driver.remove('%s/%s_temp' % (paths_dict['cube_tmp'],str(datetime.datetime.today().date())))
        #cluster.driver.remove('%s/%s_to_validate' % (paths_dict['funnel_cube'],str(datetime.datetime.today().date())))

        tables_to_save = date_range_by_days(str(datetime.date.today() - datetime.timedelta(days = 14)), str(datetime.date.today())) + ['cube', 'validation']
        tables_to_save = [paths_dict['cube_tmp'] + '/' + table for table in tables_to_save]
        for table in get_table_list(paths_dict['cube_tmp'], job).replace('{', '').replace('}', '').split(','):
            if table not in tables_to_save:
                cluster.driver.remove(table)

        text = ['''
        Acquisition Cube: Success at {0}
        '''.format(datetime.datetime.now())]
        for col in diff_col:
            text.append('''
            Have diff in {0} = {1}
            '''.format(col, diff_col[col]))
        text.append('=============================================')

        #bot.send_message(telebot_creds['value']['chat_id'], '\n'.join(text))
        requests.post('https://api.telegram.org/bot{0}/sendMessage?chat_id={1}&text={2}'.format(
            telebot_creds['value']['token'],
            telebot_creds['value']['chat_id'],
            '\n'.join(text)
            )
        )
    else:
        text = ['''
        Acquisition Cube: Fail at {0}
        '''.format(datetime.datetime.now())]
        for col in diff_col:
            text.append('''
            Have diff in {0} = {1}
            '''.format(col, diff_col[col]))
        text.append('=============================================')
        #bot.send_message(telebot_creds['value']['chat_id'], '\n'.join(text))
        requests.post('https://api.telegram.org/bot{0}/sendMessage?chat_id={1}&text={2}'.format(
            telebot_creds['value']['token'],
            telebot_creds['value']['chat_id'],
            '\n'.join(text)
            )
        )

if __name__ == '__main__':
    main()
