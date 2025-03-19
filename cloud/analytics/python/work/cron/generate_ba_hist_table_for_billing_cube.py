#!/usr/bin/env python2
# -*- coding: utf-8 -*-
import pandas as pd, datetime, ast, numpy as np, telebot, json, requests, os, sys, time
from sandbox.common import rest
from sandbox.common.auth import OAuth
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
    Record,
    files as nfi
)
from qb2.api.v1 import (
    filters as qf,
    resources as qr,
    extractors as qe,
    typing as qt,
)
from creds import (
    yt_creds,
    metrika_creds,
    yc_ch_creds,
    crm_sql_creds,
    stat_creds,
    telebot_creds,
    sandbox_creds
)
from acquisition_cube_queries import (
    queries_test,
    queries_prod
)

from acquisition_cube_init_funnel_steps_preprod import (
    paths_dict_prod,
    paths_dict_test,
    schema_cube as schema
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
            return str(metadata['block_reason']).lower()
        #if 'fraud_detected_by' in metadata:
            #if isinstance(metadata['fraud_detected_by'], list):
                #return metadata['fraud_detected_by'][0]
            #else:
                #return metadata['fraud_detected_by'].replace('[', '').replace(']', '').replace("u'", '').replace("'", '')
    return 'unknown'

def get_unblock_reason(metadata_):
    if metadata_ not in ['', None]:
        metadata =  json.loads(metadata_)
    else:
        metadata = None

    if metadata:
        if 'unblock_reason' in metadata:
            return str(metadata['unblock_reason']).lower()
        #if 'fraud_detected_by' in metadata:
            #if isinstance(metadata['fraud_detected_by'], list):
                #return metadata['fraud_detected_by'][0]
            #else:
                #return metadata['fraud_detected_by'].replace('[', '').replace(']', '').replace("u'", '').replace("'", '')
    return ''

def get_verified_flag(metadata_):
    if metadata_ not in ['', None]:
        metadata =  json.loads(metadata_)
    else:
        metadata = None

    if metadata:
        if 'verified' in metadata:
            return str(metadata['verified']).lower()
        #if 'fraud_detected_by' in metadata:
            #if isinstance(metadata['fraud_detected_by'], list):
                #return metadata['fraud_detected_by'][0]
            #else:
                #return metadata['fraud_detected_by'].replace('[', '').replace(']', '').replace("u'", '').replace("'", '')
    return ''

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

def apply_quotes(records):
    currency_rates = qr.json('currency_rates.json')()

    for record in records:
        record = record.to_dict()
        if 'currency' in record:
            record['usd_rur_quote'] = currency_rates[record['date']]['USD']
            if record['currency'] != 'USD':
                record['cost'] = float(record['cost'])
                record['credit'] = float(record['credit'])
            else:
                record['cost'] = float(record['cost'])*currency_rates[record['date']]['USD']
                record['credit'] = float(record['credit'])*currency_rates[record['date']]['USD']
        yield Record(**record)

def apply_quotes_transaction(records):
    currency_rates = qr.json('currency_rates.json')()

    for record in records:
        record = record.to_dict()
        if 'currency' in record:
            record['usd_rur_quote'] = currency_rates[record['date']]['USD']
            if record['currency'] != 'USD':
                record['amount'] = float(record['amount'])
            else:
                record['amount'] = float(record['amount'])*currency_rates[record['date']]['USD']
        yield Record(**record)

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

        last_visit_dict = {}
        last_direct_visit_dict = {}
        funnel_steps = {}
        record_list = []
        for rec in records:

            if not rec['event_time']:
                continue

            result_dict = rec.to_dict().copy()

            if not rec['event'] in funnel_steps:
                funnel_steps[rec['event']] = rec['event_time']

            for metric in metrics:
                if metric in result_dict:
                    result_dict[metric] = convert_metric_to_float(result_dict[metric])
                else:
                    result_dict[metric] = 0.0

                if result_dict['name'] and metric in result_dict:
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
        ba_created_event = {}
        for rec_dict in record_list:
            for event in funnel_steps:
                rec_dict['first_' + event + '_datetime'] = funnel_steps[event]

            if rec_dict['event'] == 'ba_created':
                ba_created_event = rec_dict.copy()

            if rec_dict['event'] == 'day_use':
                if ba_created_event:
                    rec_dict['channel'] = ba_created_event['channel']
                    rec_dict['utm_source'] = ba_created_event['utm_source']
                    rec_dict['utm_medium'] = ba_created_event['utm_medium']
                    rec_dict['utm_campaign'] = ba_created_event['utm_campaign']
                    rec_dict['utm_content'] = ba_created_event['utm_content']
                    rec_dict['utm_term'] = ba_created_event['utm_term']
                else:
                    rec_dict['channel'] = 'unknown'
                    rec_dict['utm_source'] = 'unknown'
                    rec_dict['utm_medium'] = 'unknown'
                    rec_dict['utm_campaign'] = 'unknown'
                    rec_dict['utm_content'] = 'unknown'
                    rec_dict['utm_term'] = 'unknown'

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
def execute_query(query, cluster, alias, token, timeout=1200):
    proxy = "http://{}.yt.yandex.net".format(cluster)
    s = requests.Session()
    url = "{proxy}/query?database={alias}&password={token}".format(proxy=proxy, alias=alias, token=token)
    resp = s.post(url, data=query, timeout=timeout)
    resp.raise_for_status()
    rows = resp.content.strip().split('\n')
    return rows

def chyt_execute_query(
    query,
    cluster,
    alias,
    token,
    columns,
    create_table_dict = {}
):
    i = 0
    while True:
        try:
            if 'create table' in query.lower() or 'insert into' in query.lower():
                if create_table_dict:
                    delete_table(
                        create_table_dict['path'],
                        create_table_dict['tables_dir'],
                        create_table_dict['cluster'],
                        create_table_dict['cluster'].job()
                    )
                    execute_query(query=query, cluster=cluster, alias=alias, token=token)
                else:
                    execute_query(query=query, cluster=cluster, alias=alias, token=token)
                return 'Success!!!'
            else:
                result = execute_query(query=query, cluster=cluster, alias=alias, token=token)
                users = pd.DataFrame([row.split('\t') for row in result], columns = columns)
                return users
        except Exception as err:
            if 'IncompleteRead' in  str(err.message):
                return 'Success!!!'
            print(err.message)
            i += 1
            time.sleep(5)
            if i > 30:
                print(query)
                raise ValueError('Bad Query!!!')

def get_yt_table_schema(path, cluster_):
    data = cluster_.driver.get_attribute(path,'schema')
    schema_ = {}
    for col in data:
        if 'int' in str(col['type']).lower():
            schema_[col['name']] = int
        elif 'float' in str(col['type']).lower():
            schema_[col['name']] = float
        else:
            schema_[col['name']] = str
    return schema_

def make_query_for_union(cube_schema, schema, table):
    res_ = 'SELECT\n'
    for col in sorted(cube_schema):
        if col in schema:
            res_ = res_ + '\t' + col+',\n'
        else:
            res_ = res_+ '\t' + 'NULL AS ' + col +',\n'
    res_ = res_[:-2] + '\nFROM "{0}"\n'.format(table)
    return res_

def get_result_schema(table_list, cluster_):
    res_ = []
    for table in table_list:
        res_ = res_ + list(get_yt_table_schema(table, cluster_).keys())
    return set(res_)

def delete_table(path_, tables_dir_, cluster_yt_, job_):
    if path_ in get_table_list(tables_dir_, job_)[1:-1].split(','):
        counter = 0
        while True:
            time.sleep(2)
            try:
                cluster_yt_.driver.remove(path_)
                break
            except Exception as err:
                print(err)
                counter += 1
                if counter == 15:
                    break

def get_last_success_task(response):
    if response.status_code == 200:
        result = response.json()
        tasks_dict = {}
        start_times = []
        for task in result['items']:
            if task['description'] == 'Acquisition Cube to CH':
                start_times.append(task['execution']['finished'])
                tasks_dict[task['execution']['finished']] = task['id']
        max_success = max(start_times)
        return tasks_dict[max_success]

def main():
    cluster_chyt = 'hahn'
    alias = "*cloud_analytics"
    #alias = "*ch_public"
    token_chyt = '%s' % (yt_creds['value']['token'])

    #files_list = 'get_source_visit_data_prod.py,get_calls_prod.py,get_clicked_emails_prod.py,get_ba_hist_prod_.py'
    #wait_for_done_running_proccess(os, files_list)

    mode = 'prod'
    bot = telebot.TeleBot(telebot_creds['value']['token'])
    cluster = clusters.yt.Hahn(
        token = yt_creds['value']['token'],
        pool = yt_creds['value']['pool']
    )
    if mode == 'test':
        paths_dict_temp = paths_dict_test
        queries = queries_test
        folder = 'cloud_analytics_test'
        tables_cube_dir = "//home/{0}/cubes/preprod/acquisition_cube".format(folder)
        tables_cube_tmp_dir = "//home/{0}/cooking_cubes/acquisition_cube/preprod/sources/tmp".format(folder)
    elif mode == 'prod':
        paths_dict_temp = paths_dict_prod
        queries = queries_prod
        folder = 'cloud_analytics'
        tables_cube_dir = "//home/{0}/cubes/preprod/acquisition_cube".format(folder)
        tables_cube_tmp_dir = "//home/{0}/cooking_cubes/acquisition_cube/preprod/sources/tmp".format(folder)
    paths_dict = paths_dict_temp.copy()
    job = cluster.job()

    last_tables = ['billing_grants','cloud_owner_path', 'billing_accounts_path', 'sku_path', 'service_dict_path', 'billing_accounts_path', 'billing_records_path', 'billing_accounts_history_path']
    for path in last_tables:
        valid_path = get_last_not_empty_table(paths_dict_temp[path], job)
        print(valid_path)
        paths_dict[path] = valid_path


    query = '''
    CREATE TABLE "//home/{1}/cooking_cubes/acquisition_cube/preprod/sources/tmp/clouds" ENGINE = YtTable() AS
    SELECT
        t0.*,
        multiIf(t1.is_corporate_card IS NULL, 0, t1.is_corporate_card) as is_corporate_card
    FROM(
        SELECT
            passport_uid as puid,
            'cloud_created' as event,
            argMin(email, cloud_created_at) as email,
            argMin(first_name, cloud_created_at) as first_name,
            argMin(last_name, cloud_created_at) as last_name,
            argMin(login, cloud_created_at) as login,
            argMin(phone, cloud_created_at) as phone,
            argMin(user_settings_email, cloud_created_at) as user_settings_email,
            argMin(cloud_status, cloud_created_at) as cloud_status,
            argMin(cloud_name, cloud_created_at) as cloud_name,
            argMin(toString(toDateTime(replaceAll(splitByString('+', toString(assumeNotNull(cloud_created_at)))[1], 'T', ' '))), cloud_created_at) as event_time,
            argMin(cloud_id, cloud_created_at) as cloud_id,
            argMin(mail_tech, cloud_created_at) as mail_tech,
            argMin(mail_testing, cloud_created_at) as mail_testing,
            argMin(mail_info, cloud_created_at) as mail_info,
            argMin(mail_feature, cloud_created_at) as mail_feature,
            argMin(mail_event, cloud_created_at) as mail_event,
            argMin(mail_promo, cloud_created_at) as mail_promo,
            argMin(mail_billing, cloud_created_at) as mail_billing
        FROM "{0}"
        GROUP BY
            puid,
            event
    ) as t0
    ANY LEFT JOIN(
        SELECT
            puid,
            MAX(is_corporate_card) as is_corporate_card
        FROM(
            SELECT
                DISTINCT
                "card_user_account",
                toInt64(splitByString('*', assumeNotNull(card_user_account))[1]) as bin,
                multiIf(bin IN (SELECT DISTINCT bin FROM "//home/cloud_analytics/dictionaries/corporate_cards/bins" WHERE payment_system = 'mastercard'), 1, 0) as is_corporate_card,
                toString(creator_uid) as puid
            FROM "//home/cloud_analytics/import/payments/payment"
            WHERE
                card_user_account IS NOT NULL
        )
        GROUP BY
            puid
    ) as t1
    ON t0.puid = t1.puid
    '''.format(paths_dict['cloud_owner_path'], folder)

    path = '//home/{0}/cooking_cubes/acquisition_cube/preprod/sources/tmp/clouds'.format(folder)

    chyt_execute_query(
        query=query,
        cluster=cluster_chyt,
        alias=alias,
        token=token_chyt,
        columns = [],
        create_table_dict = {
            'path': path,
            'tables_dir': tables_cube_tmp_dir,
            'cluster': cluster
        }
    )
    print("Done for {0}".format('//home/{0}/cooking_cubes/acquisition_cube/preprod/sources/tmp/clouds'.format(folder)))

    query = '''
    CREATE TABLE "//home/{1}/cooking_cubes/acquisition_cube/preprod/sources/tmp/offers" ENGINE = YtTable() AS
    SELECT
        passport_uid as puid,
        MAX(client_name) as promocode_client_name,
        MAX(type) as promocode_type,
        MAX(client_source) as promocode_source,
        MAX(multiIf( client_source != 'direct_offer', 'Enterprise/ISV', 'Direct')) as promocode_client_type
    FROM "{0}"
    WHERE
        puid != '' AND puid IS NOT NULL
    GROUP BY
        puid
    '''.format(paths_dict['offers_path'], folder)

    path = '//home/{0}/cooking_cubes/acquisition_cube/preprod/sources/tmp/offers'.format(folder)

    chyt_execute_query(
        query=query,
        cluster=cluster_chyt,
        alias=alias,
        token=token_chyt,
        columns = [],
        create_table_dict = {
            'path': path,
            'tables_dir': tables_cube_tmp_dir,
            'cluster': cluster
        }
    )
    print("Done for {0}".format('//home/{0}/cooking_cubes/acquisition_cube/preprod/sources/tmp/offers'.format(folder)))

    query = '''
    CREATE TABLE "//home/{1}/cooking_cubes/acquisition_cube/preprod/sources/tmp/segments" ENGINE = YtTable() AS
    SELECT
        billing_account_id,
        argMax(segment, date) as segment,
        argMax(architect, date) architect,
        argMax(is_iot, date) is_iot,
        argMax(is_var, date) is_var,
        argMax(is_isv, date) is_isv,
        argMax(account_name, date) account_name,
        argMax(potential, date) potential,
        argMax(master_account_id, date) master_account_id,
        argMax(sales_name, date) sales_name,
        argMax(is_fraud, date) is_fraud,
        argMax(board_segment, date) board_segment,
        argMax(grant_sources, date) grant_sources,
        argMax(grant_amounts, date) grant_amounts,
        argMax(grant_amount, date) grant_amount,
        argMax(grant_source_ids, date) grant_source_ids,
        argMax(grant_start_times, date) grant_start_times,
        argMax(grant_end_times, date) grant_end_times,
        argMax(active_grant_ids, date) active_grant_ids,
        argMax(active_grants, date) active_grants
    FROM "{0}"
    GROUP BY
        billing_account_id
    '''.format(paths_dict['ba_hist'], folder)
    path = '//home/{0}/cooking_cubes/acquisition_cube/preprod/sources/tmp/segments'.format(folder)
    chyt_execute_query(
        query=query,
        cluster=cluster_chyt,
        alias=alias,
        token=token_chyt,
        columns = [],
        create_table_dict = {
            'path': path,
            'tables_dir': tables_cube_tmp_dir,
            'cluster': cluster
        }
    )
    print("Done for {0}".format('//home/{0}/cooking_cubes/acquisition_cube/preprod/sources/tmp/segments'.format(folder)))

    query = '''
    CREATE TABLE "//home/{1}/cooking_cubes/acquisition_cube/preprod/sources/tmp/passports" ENGINE = YtTable() AS
    SELECT
        PASSPORT_ID as puid,
        MAX(NAME) as balance_name,
        MAX(LONGNAME) as balance_long_name
    FROM "{0}"
    GROUP BY
        puid
    '''.format(paths_dict['balance'], folder)
    path = '//home/{0}/cooking_cubes/acquisition_cube/preprod/sources/tmp/passports'.format(folder)
    chyt_execute_query(
        query=query,
        cluster=cluster_chyt,
        alias=alias,
        token=token_chyt,
        columns = [],
        create_table_dict = {
            'path': path,
            'tables_dir': tables_cube_tmp_dir,
            'cluster': cluster
        }
    )
    print("Done for {0}".format('//home/{0}/cooking_cubes/acquisition_cube/preprod/sources/tmp/passports'.format(folder)))

    query = '''
    CREATE TABLE "//home/{1}/cooking_cubes/acquisition_cube/preprod/sources/tmp/ba_puid_dict_" ENGINE = YtTable() AS
    SELECT
        ba_of_se.*,
        balance_name,
        balance_long_name
    FROM(
        SELECT
            ba_of.*,
            multiIf(segment IS NULL OR segment = '', 'Mass', segment) as segment,
            architect,
            is_iot,
            is_isv,
            is_var,
            account_name,
            potential,
            master_account_id,
            sales_name,
            is_fraud,
            board_segment,
            grant_sources,
            grant_amounts,
            grant_amount,
            grant_source_ids,
            grant_start_times,
            grant_end_times,
            active_grant_ids,
            active_grants
        FROM(
            SELECT
                ba.*,
                promocode_client_name,
                promocode_type,
                promocode_source,
                promocode_client_type
            FROM(
                SELECT
                    id as billing_account_id,
                    created_at as ba_created_at,
                    name as ba_name,
                    state as ba_state,
                    person_type as ba_person_type,
                    payment_cycle_type as ba_payment_cycle_type,
                    usage_status as ba_usage_status,
                    currency as ba_currency,
                    type as ba_type,
                    owner_id as puid,
                    replaceRegexpAll(extract(metadata, '"block_reason": "[a-z _]+"'), '("block_reason": "|")', '') as block_reason,
                    replaceRegexpAll(extract(metadata, '"unblock_reason": "[a-z _]+"'), '("unblock_reason": "|")', '') as unblock_reason,
                    toFloat64(balance) as balance,
                    replaceRegexpAll(extract(metadata, '"verified": [a-z]+'), '(verified|:| |")', '') as is_verified
                FROM "{0}"
            ) as ba
            ANY LEFT JOIN(
                SELECT
                    puid,
                    promocode_client_name,
                    promocode_type,
                    promocode_source,
                    promocode_client_type
                FROM "//home/{1}/cooking_cubes/acquisition_cube/preprod/sources/tmp/offers"
            ) as of
            ON ba.puid = of.puid
        ) as ba_of
        ANY LEFT JOIN(
            SELECT
                billing_account_id,
                segment,
                architect,
                is_iot,
                is_isv,
                is_var,
                account_name,
                potential,
                master_account_id,
                sales_name,
                is_fraud,
                board_segment,
                grant_sources,
                grant_amounts,
                grant_amount,
                grant_source_ids,
                grant_start_times,
                grant_end_times,
                active_grant_ids,
                active_grants
            FROM "//home/{1}/cooking_cubes/acquisition_cube/preprod/sources/tmp/segments"
        ) as se
        ON ba_of.billing_account_id = se.billing_account_id
    ) as ba_of_se
    ANY LEFT JOIN(
        SELECT
            puid,
            balance_name,
            balance_long_name
        FROM "//home/{1}/cooking_cubes/acquisition_cube/preprod/sources/tmp/passports"
    ) as pa
    ON ba_of_se.puid = pa.puid
    '''.format(paths_dict['billing_accounts_path'], folder)
    path = '//home/{0}/cooking_cubes/acquisition_cube/preprod/sources/tmp/ba_puid_dict_'.format(folder)
    chyt_execute_query(
        query=query,
        cluster=cluster_chyt,
        alias=alias,
        token=token_chyt,
        columns = [],
        create_table_dict = {
            'path': path,
            'tables_dir': tables_cube_tmp_dir,
            'cluster': cluster
        }
    )
    print("Done for {0}".format('//home/{0}/cooking_cubes/acquisition_cube/preprod/sources/tmp/ba_puid_dict_'.format(folder)))

    query = '''
    CREATE TABLE "//home/{1}/cooking_cubes/acquisition_cube/preprod/sources/tmp/ba_puid_dict_hist" ENGINE = YtTable() AS
    SELECT
        ba_of_cl.*,
        balance_name,
        balance_long_name
    FROM(
        SELECT
            ba_of.*,
            email,
            first_name,
            last_name,
            login,
            phone,
            user_settings_email,
            cloud_status,
            cloud_name,
            cloud_id,
            puid,
            mail_tech,
            mail_testing,
            mail_info,
            mail_feature,
            mail_event,
            mail_promo,
            mail_billing,
            is_corporate_card
        FROM(
            SELECT
                ba.*,
                promocode_client_name,
                promocode_type,
                promocode_source,
                promocode_client_type
            FROM(
                SELECT
                    billing_account_id as billing_account_id,
                    date,
                    name as ba_name,
                    state as ba_state,
                    name as ba_name,
                    state as ba_state,
                    person_type as ba_person_type,
                    payment_cycle_type as ba_payment_cycle_type,
                    usage_status as ba_usage_status,
                    currency as ba_currency,
                    type as ba_type,
                    owner_id as puid,
                    block_reason as block_reason,
                    sales_name as sales_name,
                    is_var as is_var,
                    is_isv as is_isv,
                    architect as architect,
                    is_iot as is_iot,
                    multiIf(segment IS NULL OR segment = '', 'Mass', segment) as segment,
                    potential as potential,
                    account_name as account_name,
                    master_account_id as master_account_id,
                    toFloat64(balance) as balance,
                    is_fraud as is_fraud,
                    board_segment as board_segment,
                    is_verified as is_verified,
                    unblock_reason as unblock_reason,
                    grant_sources as grant_sources,
                    grant_amounts as grant_amounts,
                    grant_amount as grant_amount,
                    grant_source_ids as grant_source_ids,
                    grant_start_times as grant_start_times,
                    grant_end_times as grant_end_times,
                    active_grant_ids as active_grant_ids,
                    active_grants as active_grants
                FROM "{0}"
            ) as ba
            ANY LEFT JOIN(
                SELECT
                    puid,
                    promocode_client_name,
                    promocode_type,
                    promocode_source,
                    promocode_client_type
                FROM "//home/{1}/cooking_cubes/acquisition_cube/preprod/sources/tmp/offers"
            ) as of
            ON ba.puid = of.puid
        ) as ba_of
        ANY LEFT JOIN(
            SELECT
                email,
                first_name,
                last_name,
                login,
                phone,
                user_settings_email,
                cloud_status,
                cloud_name,
                cloud_id,
                puid,
                mail_tech,
                mail_testing,
                mail_info,
                mail_feature,
                mail_event,
                mail_promo,
                mail_billing,
                is_corporate_card
            FROM "//home/{1}/cooking_cubes/acquisition_cube/preprod/sources/tmp/clouds"
        ) as cl
        ON ba_of.puid = cl.puid
    ) as ba_of_cl
    ANY LEFT JOIN(
        SELECT
            puid,
            balance_name,
            balance_long_name
        FROM "//home/{1}/cooking_cubes/acquisition_cube/preprod/sources/tmp/passports"
    ) as pa
    ON ba_of_cl.puid = pa.puid
    '''.format(paths_dict['ba_hist'], folder)
    path = '//home/{0}/cooking_cubes/acquisition_cube/preprod/sources/tmp/ba_puid_dict_hist'.format(folder)
    chyt_execute_query(
        query=query,
        cluster=cluster_chyt,
        alias=alias,
        token=token_chyt,
        columns = [],
        create_table_dict = {
            'path': path,
            'tables_dir': tables_cube_tmp_dir,
            'cluster': cluster
        }
    )
    print("Done for {0}".format('//home/{0}/cooking_cubes/acquisition_cube/preprod/sources/tmp/ba_puid_dict_hist'.format(folder)))

    query = '''
    CREATE TABLE "//home/{0}/cooking_cubes/acquisition_cube/preprod/for_billing_cube/ba_puid_dict_hist_" ENGINE = YtTable() AS
    SELECT
        t0.*,
        t1.sales_name_actual,
        t1.is_var_actual,
        t1.is_isv_actual,
        t1.architect_actual,
        t1.is_iot_actual,
        t1.segment_actual,
        t1.potential_actual,
        t1.board_segment_actual
    FROM(
        SELECT
            billing_account_id,
            date,
            ba_state as ba_state_at_moment,
            ba_person_type as ba_person_type_at_moment,
            ba_payment_cycle_type as ba_payment_cycle_type_at_moment,
            ba_usage_status as ba_usage_status_at_moment,
            ba_type as ba_type_at_moment_at_moment,
            puid,
            block_reason as block_reason_at_moment,
            sales_name as sales_name_at_moment,
            is_var as is_var_at_moment,
            is_isv as is_isv_at_moment,
            architect as architect_at_moment,
            is_iot as is_iot_at_moment,
            segment as segment_at_moment,
            potential as potential_at_moment,
            multiIf(
                account_name IS NOT NULL AND account_name != 'unknown', account_name,
                master_account_id IS NOT NULL AND master_account_id != '', ba_name,
                balance_name IS NOT NULL, balance_name,
                ba_name IS NOT NULL, ba_name,
                CONCAT(first_name, ' ', last_name)
            ) as account_name,
            master_account_id,
            balance,
            is_fraud,
            board_segment as board_segment_at_moment,
            is_verified as is_verified_at_moment,
            unblock_reason as unblock_reason_at_moment,
            grant_sources,
            grant_amounts,
            grant_amount,
            grant_source_ids,
            grant_start_times,
            grant_end_times,
            active_grant_ids,
            active_grants,
            email,
            first_name,
            last_name,
            login,
            phone,
            user_settings_email,
            is_corporate_card,
            balance_name,
            balance_long_name
        FROM "//home/{0}/cooking_cubes/acquisition_cube/preprod/sources/tmp/ba_puid_dict_hist"
    ) as t0
    ANY LEFT JOIN(
            SELECT
            billing_account_id,
            argMax(sales_name, date) as sales_name_actual,
            argMax(is_var, date) as is_var_actual,
            argMax(is_isv, date) as is_isv_actual,
            argMax(architect, date) as architect_actual,
            argMax(is_iot, date) as is_iot_actual,
            argMax(segment, date) as segment_actual,
            argMax(potential, date) as potential_actual,
            argMax(board_segment, date) as board_segment_actual
        FROM "//home/{0}/cooking_cubes/acquisition_cube/preprod/sources/tmp/ba_puid_dict_hist"
        GROUP BY
            billing_account_id
    ) as t1
    ON t0.billing_account_id = t1.billing_account_id
    '''.format(folder)
    path = '//home/{0}/cooking_cubes/acquisition_cube/preprod/for_billing_cube/ba_puid_dict_hist_'.format(folder)
    chyt_execute_query(
        query=query,
        cluster=cluster_chyt,
        alias=alias,
        token=token_chyt,
        columns = [],
        create_table_dict = {
            'path': path,
            'tables_dir': '//home/{0}/cooking_cubes/acquisition_cube/preprod/for_billing_cube'.format(folder),
            'cluster': cluster
        }
    )
    print("Done for {0}".format('//home/{0}/cooking_cubes/acquisition_cube/preprod/for_billing_cube/ba_puid_dict_hist_'.format(folder)))

    try:
        cluster.driver.remove('//home/{0}/cooking_cubes/acquisition_cube/preprod/for_billing_cube/ba_puid_dict_hist'.format(folder))
    except:
        pass
    cluster.driver.copy(
        '//home/{0}/cooking_cubes/acquisition_cube/preprod/for_billing_cube/ba_puid_dict_hist_'.format(folder),
        '//home/{0}/cooking_cubes/acquisition_cube/preprod/for_billing_cube/ba_puid_dict_hist'.format(folder)
    )
    cluster.driver.remove('//home/{0}/cooking_cubes/acquisition_cube/preprod/for_billing_cube/ba_puid_dict_hist_'.format(folder))

if __name__ == '__main__':
    main()
