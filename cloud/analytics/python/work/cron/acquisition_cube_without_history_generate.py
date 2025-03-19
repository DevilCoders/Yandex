#!/usr/bin/env python2
# -*- coding: utf-8 -*-
import pandas as pd, datetime, ast, numpy as np, telebot, json, requests, os, sys, time
from sandbox.common import rest
from sandbox.common.auth import OAuth
from requests.exceptions import HTTPError
import time

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

from acquisition_cube_init_funnel_steps import (
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
        metadata = json.loads(metadata_)
    else:
        metadata = None

    if metadata:
        if 'block_reason' in metadata:
            return str(metadata['block_reason']).lower()
        # if 'fraud_detected_by' in metadata:
        # if isinstance(metadata['fraud_detected_by'], list):
        # return metadata['fraud_detected_by'][0]
        # else:
        # return metadata['fraud_detected_by'].replace('[', '').replace(']', '').replace("u'", '').replace("'", '')
    return 'unknown'


def get_unblock_reason(metadata_):
    if metadata_ not in ['', None]:
        metadata = json.loads(metadata_)
    else:
        metadata = None

    if metadata:
        if 'unblock_reason' in metadata:
            return str(metadata['unblock_reason']).lower()
        # if 'fraud_detected_by' in metadata:
        # if isinstance(metadata['fraud_detected_by'], list):
        # return metadata['fraud_detected_by'][0]
        # else:
        # return metadata['fraud_detected_by'].replace('[', '').replace(']', '').replace("u'", '').replace("'", '')
    return ''


def get_verified_flag(metadata_):
    if metadata_ not in ['', None]:
        metadata = json.loads(metadata_)
    else:
        metadata = None

    if metadata:
        if 'verified' in metadata:
            return str(metadata['verified']).lower()
        # if 'fraud_detected_by' in metadata:
        # if isinstance(metadata['fraud_detected_by'], list):
        # return metadata['fraud_detected_by'][0]
        # else:
        # return metadata['fraud_detected_by'].replace('[', '').replace(']', '').replace("u'", '').replace("'", '')
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
        date_list.append(str((start + datetime.timedelta(days=i)).date()))
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
            last_table_rows = table_rows
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
            apply_types_dict[col] = ne.custom(
                lambda x: str(x).replace('"', '').replace("'", '').replace('\\', '') if x not in ['', None] else None,
                col)

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
        date_list.append(str((start + datetime.timedelta(days=i)).date()))

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
        return (datetime.datetime.strptime(new_date, '%Y-%m-%d %H:%M:%S') - datetime.datetime.strptime(old_date,
                                                                                                       '%Y-%m-%d %H:%M:%S')).seconds

    except:
        pass


def apply_attr(result_dict_, last_visit_dict_):
    for col in last_visit_dict_:
        try:
            result_dict_[col] = last_visit_dict_[col]

        except:
            result_dict_[col] = None

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
                record['cost'] = float(record['cost']) * currency_rates[record['date']]['USD']
                record['credit'] = float(record['credit']) * currency_rates[record['date']]['USD']
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
                record['amount'] = float(record['amount']) * currency_rates[record['date']]['USD']
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
                # metrics[metric] += result_dict[metric]
                try:
                    result_dict[metric + '_cum'] = metric_sku[result_dict['name'] + '_' + metric]
                except:
                    result_dict[metric + '_cum'] = 0.0

            if is_first_event == 1:

                is_first_event = 0

                if rec['event'] not in ['visit', 'day_use', 'call', 'click_mail']:

                    for utm in utms:
                        result_dict[utm] = 'Unknown'

                    # yield Record(key, **result_dict)
                    record_list.append(result_dict)

                else:
                    if rec['event'] in ('visit', 'call', 'click_mail'):

                        if 'cloud.yandex' in str(rec['referer']):
                            continue

                        for visit_col in visits_settings:
                            last_visit_dict[visit_col] = rec[visit_col]

                        last_visit_dict['session_start_time'] = rec['event_time']

                        if (last_visit_dict['referer'] not in ['', None] and 'cloud.' not in last_visit_dict[
                            'referer'] and 'Organic' not in last_visit_dict['channel']) or rec['event'] in ['call',
                                                                                                            'click_mail']:

                            for visit_col in visits_settings:
                                last_direct_visit_dict[visit_col] = rec[visit_col]

                            last_direct_visit_dict['session_start_time'] = rec['event_time']

                    # yield Record(key, **result_dict)
                    record_list.append(result_dict)

            else:

                if rec['event'] not in ['visit', 'day_use', 'call', 'click_mail']:

                    if last_direct_visit_dict or last_visit_dict:

                        if last_direct_visit_dict:
                            if get_time_delta(result_dict['event_time'],
                                              last_direct_visit_dict['session_start_time']) <= attr_window:
                                result_dict = apply_attr(result_dict, last_direct_visit_dict)

                        elif last_visit_dict:
                            if get_time_delta(result_dict['event_time'],
                                              last_visit_dict['session_start_time']) <= attr_window:
                                result_dict = apply_attr(result_dict, last_visit_dict)
                            else:
                                for utm in utms:
                                    result_dict[utm] = 'Unknown'

                    else:
                        for utm in utms:
                            result_dict[utm] = 'Unknown'

                    # yield Record(key, **result_dict)
                    record_list.append(result_dict)

                else:

                    if rec['event'] in ('visit', 'call', 'click_mail'):
                        if 'cloud.yandex' in str(rec['referer']):
                            continue

                        for visit_col in visits_settings:
                            last_visit_dict[visit_col] = rec[visit_col]

                        last_visit_dict['session_start_time'] = rec['event_time']

                        if (last_visit_dict['referer'] not in ['', None] and 'cloud.' not in last_visit_dict[
                            'referer'] and 'Organic' not in last_visit_dict['channel']) or rec['event'] in ['call',
                                                                                                            'click_mail']:

                            for visit_col in visits_settings:
                                last_direct_visit_dict[visit_col] = rec[visit_col]

                            last_direct_visit_dict['session_start_time'] = rec['event_time']

                    # yield Record(key, **result_dict)
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
    # lst = lst.split('\n')
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
    try:
        resp = s.post(url, data=query, timeout=timeout)
        resp.raise_for_status()
    except HTTPError as err:
        if resp.text:
            raise HTTPError('{} Error Message: {}'.format(str(err.message), resp.text))
        else:
            raise err
    rows = resp.content.strip().split('\n')
    return rows


def chyt_execute_query(
    query,
    cluster,
    alias,
    token,
    columns,
    create_table_dict={}
):
    print("sleeping 20 seconds to avoid chyt lag")
    time.sleep(20)
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
                users = pd.DataFrame([row.split('\t') for row in result], columns=columns)
                return users
        except Exception as err:
            if 'IncompleteRead' in str(err.message):
                return 'Success!!!'
            print(err.message)
            i += 1
            time.sleep(5)
            if i > 30:
                print(query)
                raise ValueError('Bad Query!!!')


def get_yt_table_schema(path, cluster_):
    data = cluster_.driver.get_attribute(path, 'schema')
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
            res_ = res_ + '\t' + col + ',\n'
        else:
            res_ = res_ + '\t' + 'NULL AS ' + col + ',\n'
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
    # alias = "*ch_public"
    token_chyt = '%s' % (yt_creds['value']['token'])

    files_list = 'get_source_visit_data_prod.py,get_calls_prod.py,get_clicked_emails_prod.py,get_ba_hist_prod_.py'
    wait_for_done_running_proccess(os, files_list)

    mode = 'prod'
    bot = telebot.TeleBot(telebot_creds['value']['token'])
    cluster = clusters.yt.Hahn(
        token=yt_creds['value']['token'],
        pool=yt_creds['value']['pool']
    )
    if mode == 'test':
        paths_dict_temp = paths_dict_test
        queries = queries_test
        folder = 'cloud_analytics_test'
        tables_cube_dir = "//home/{0}/cubes/acquisition_cube".format(folder)
        tables_cube_tmp_dir = "//home/{0}/cooking_cubes/acquisition_cube/without_history_part/sources/tmp".format(
            folder)
    elif mode == 'prod':
        paths_dict_temp = paths_dict_prod
        queries = queries_prod
        folder = 'cloud_analytics'
        tables_cube_dir = "//home/{0}/cubes/acquisition_cube".format(folder)
        tables_cube_tmp_dir = "//home/{0}/cooking_cubes/acquisition_cube/without_history_part/sources/tmp".format(
            folder)
    paths_dict = paths_dict_temp.copy()

    # job = cluster.job()
    # last_tables = ['transactions_path']
    # for path in last_tables:
    # valid_path = get_last_not_empty_table(paths_dict_temp[path], job)
    # print(valid_path)
    # paths_dict[path] = valid_path
    # check tables in folders
    tables = []

    '''
    now = str(datetime.date.today()) + ' 04:00:00'
    for key in last_tables:
        table_date = str(paths_dict[key].split('/')[-1].replace('T', ' '))
        if ':' not in table_date:
            if table_date.split(' ')[0] != now.split(' ')[0]:
                tables.append(paths_dict[key])
        else:
            if table_date < now:
                tables.append(paths_dict[key])
    '''
    if tables:
        text = ['''
        Acquisition {0} Cube:\nWhere is no resent data in folders:
        '''.format(mode)]
        text = text + tables
        text.append('=============================================')
        # bot.send_message(telebot_creds['value']['chat_id'], '\n'.join(text))
        requests.post('https://api.telegram.org/bot{0}/sendMessage?chat_id={1}&text={2}'.format(
            telebot_creds['value']['token'],
            telebot_creds['value']['chat_id'],
            '\n'.join(text)
        )
        )

    paths_dict['cube_tmp'] = '//home/cloud_analytics/cubes/acquisition_cube_without_history_part'

    paths_dict['funnel_cube'] = '//home/cloud_analytics/cubes/acquisition_cube_without_history_part'

    query = '''
    CREATE TABLE "//home/{1}/cooking_cubes/acquisition_cube/without_history_part/sources/tmp/clouds" ENGINE = YtTable() AS
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

    path = '//home/{0}/cooking_cubes/acquisition_cube/without_history_part/sources/tmp/clouds'.format(folder)

    chyt_execute_query(
        query=query,
        cluster=cluster_chyt,
        alias=alias,
        token=token_chyt,
        columns=[],
        create_table_dict={
            'path': path,
            'tables_dir': tables_cube_tmp_dir,
            'cluster': cluster
        }
    )
    print("Done for {0}".format(
        '//home/{0}/cooking_cubes/acquisition_cube/without_history_part/sources/tmp/clouds'.format(folder)))

    query = '''
    CREATE TABLE "//home/{1}/cooking_cubes/acquisition_cube/without_history_part/sources/tmp/offers" ENGINE = YtTable() AS
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

    path = '//home/{0}/cooking_cubes/acquisition_cube/without_history_part/sources/tmp/offers'.format(folder)

    chyt_execute_query(
        query=query,
        cluster=cluster_chyt,
        alias=alias,
        token=token_chyt,
        columns=[],
        create_table_dict={
            'path': path,
            'tables_dir': tables_cube_tmp_dir,
            'cluster': cluster
        }
    )
    print("Done for {0}".format(
        '//home/{0}/cooking_cubes/acquisition_cube/without_history_part/sources/tmp/offers'.format(folder)))

    query = '''
    CREATE TABLE "//home/{1}/cooking_cubes/acquisition_cube/without_history_part/sources/tmp/segments" ENGINE = YtTable() AS
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
    path = '//home/{0}/cooking_cubes/acquisition_cube/without_history_part/sources/tmp/segments'.format(folder)
    chyt_execute_query(
        query=query,
        cluster=cluster_chyt,
        alias=alias,
        token=token_chyt,
        columns=[],
        create_table_dict={
            'path': path,
            'tables_dir': tables_cube_tmp_dir,
            'cluster': cluster
        }
    )
    print("Done for {0}".format(
        '//home/{0}/cooking_cubes/acquisition_cube/without_history_part/sources/tmp/segments'.format(folder)))

    query = '''
    CREATE TABLE "//home/{1}/cooking_cubes/acquisition_cube/without_history_part/sources/tmp/passports" ENGINE = YtTable() AS
    SELECT
        toString(passport_id) as puid,
        MAX(name) as balance_name,
        MAX(longname) as balance_long_name
    FROM "{0}"
    GROUP BY
        puid
    '''.format(paths_dict['balance'], folder)
    path = '//home/{0}/cooking_cubes/acquisition_cube/without_history_part/sources/tmp/passports'.format(folder)
    chyt_execute_query(
        query=query,
        cluster=cluster_chyt,
        alias=alias,
        token=token_chyt,
        columns=[],
        create_table_dict={
            'path': path,
            'tables_dir': tables_cube_tmp_dir,
            'cluster': cluster
        }
    )
    print("Done for {0}".format(
        '//home/{0}/cooking_cubes/acquisition_cube/without_history_part/sources/tmp/passports'.format(folder)))

    query = '''
    CREATE TABLE "//home/{1}/cooking_cubes/acquisition_cube/without_history_part/sources/tmp/ba_puid_dict_" ENGINE = YtTable() AS
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
                FROM "//home/{1}/cooking_cubes/acquisition_cube/without_history_part/sources/tmp/offers"
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
            FROM "//home/{1}/cooking_cubes/acquisition_cube/without_history_part/sources/tmp/segments"
        ) as se
        ON ba_of.billing_account_id = se.billing_account_id
    ) as ba_of_se
    ANY LEFT JOIN(
        SELECT
            puid,
            balance_name,
            balance_long_name
        FROM "//home/{1}/cooking_cubes/acquisition_cube/without_history_part/sources/tmp/passports"
    ) as pa
    ON ba_of_se.puid = pa.puid
    '''.format(paths_dict['billing_accounts_path'], folder)
    path = '//home/{0}/cooking_cubes/acquisition_cube/without_history_part/sources/tmp/ba_puid_dict_'.format(folder)
    chyt_execute_query(
        query=query,
        cluster=cluster_chyt,
        alias=alias,
        token=token_chyt,
        columns=[],
        create_table_dict={
            'path': path,
            'tables_dir': tables_cube_tmp_dir,
            'cluster': cluster
        }
    )
    print("Done for {0}".format(
        '//home/{0}/cooking_cubes/acquisition_cube/without_history_part/sources/tmp/ba_puid_dict_'.format(folder)))

    query = '''
    CREATE TABLE "//home/{1}/cooking_cubes/acquisition_cube/without_history_part/sources/tmp/ba_puid_dict_hist" ENGINE = YtTable() AS
    SELECT
        DISTINCT
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
                    active_grants as active_grants,
                    account_name
                FROM "{0}"
            ) as ba
            ANY LEFT JOIN(
                SELECT
                    puid,
                    promocode_client_name,
                    promocode_type,
                    promocode_source,
                    promocode_client_type
                FROM "//home/{1}/cooking_cubes/acquisition_cube/without_history_part/sources/tmp/offers"
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
            FROM "//home/{1}/cooking_cubes/acquisition_cube/without_history_part/sources/tmp/clouds"
        ) as cl
        ON ba_of.puid = cl.puid
    ) as ba_of_cl
    ANY LEFT JOIN(
        SELECT
            puid,
            balance_name,
            balance_long_name
        FROM "//home/{1}/cooking_cubes/acquisition_cube/without_history_part/sources/tmp/passports"
    ) as pa
    ON ba_of_cl.puid = pa.puid
    '''.format(paths_dict['ba_hist'], folder)
    path = '//home/{0}/cooking_cubes/acquisition_cube/without_history_part/sources/tmp/ba_puid_dict_hist'.format(folder)
    chyt_execute_query(
        query=query,
        cluster=cluster_chyt,
        alias=alias,
        token=token_chyt,
        columns=[],
        create_table_dict={
            'path': path,
            'tables_dir': tables_cube_tmp_dir,
            'cluster': cluster
        }
    )
    print("Done for {0}".format(
        '//home/{0}/cooking_cubes/acquisition_cube/without_history_part/sources/tmp/ba_puid_dict_hist'.format(folder)))

    query = '''
    CREATE TABLE "//home/{0}/cooking_cubes/acquisition_cube/for_billing_cube/ba_puid_dict_hist_" ENGINE = YtTable() AS
    SELECT
        DISTINCT
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
        FROM "//home/{0}/cooking_cubes/acquisition_cube/without_history_part/sources/tmp/ba_puid_dict_hist"
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
        FROM "//home/{0}/cooking_cubes/acquisition_cube/without_history_part/sources/tmp/ba_puid_dict_hist"
        GROUP BY
            billing_account_id
    ) as t1
    ON t0.billing_account_id = t1.billing_account_id
    '''.format(folder)
    path = '//home/{0}/cooking_cubes/acquisition_cube/for_billing_cube/ba_puid_dict_hist_'.format(folder)
    chyt_execute_query(
        query=query,
        cluster=cluster_chyt,
        alias=alias,
        token=token_chyt,
        columns=[],
        create_table_dict={
            'path': path,
            'tables_dir': '//home/{0}/cooking_cubes/acquisition_cube/for_billing_cube'.format(folder),
            'cluster': cluster
        }
    )
    print("Done for {0}".format(
        '//home/{0}/cooking_cubes/acquisition_cube/for_billing_cube/ba_puid_dict_hist_'.format(folder)))

    try:
        cluster.driver.remove(
            '//home/{0}/cooking_cubes/acquisition_cube/for_billing_cube/ba_puid_dict_hist'.format(folder))
    except:
        pass
    cluster.driver.copy(
        '//home/{0}/cooking_cubes/acquisition_cube/for_billing_cube/ba_puid_dict_hist_'.format(folder),
        '//home/{0}/cooking_cubes/acquisition_cube/for_billing_cube/ba_puid_dict_hist'.format(folder)
    )
    cluster.driver.remove(
        '//home/{0}/cooking_cubes/acquisition_cube/for_billing_cube/ba_puid_dict_hist_'.format(folder))

    query = '''
    CREATE TABLE "//home/{1}/cooking_cubes/acquisition_cube/without_history_part/sources/tmp/sku_dict" ENGINE = YtTable() AS
    SELECT
        t0.*,
        service_long_name,
        subservice_name,
        platform,
        service_name,
        service_group,
        core_fraction,
        database,
        sku_name,
        sku_lazy,
        preemptible
    FROM(
        SELECT
            id as sku_id,
            name,
            service_id,
            pricing_unit,
            usage_unit,
            usage_type
        FROM "{0}"
    ) as t0
    ANY LEFT JOIN(
        SELECT
            sku_id,
            service_long_name,
            subservice_name,
            platform,
            service_name,
            service_group,
            core_fraction,
            database,
            sku_name,
            sku_lazy,
            preemptible
        FROM "//home/cloud_analytics/export/billing/sku_tags/sku_tags"
    ) as t1
    ON t0.sku_id = t1.sku_id
    '''.format(paths_dict['sku_path'], folder)
    path = '//home/{0}/cooking_cubes/acquisition_cube/without_history_part/sources/tmp/sku_dict'.format(folder)
    chyt_execute_query(
        query=query,
        cluster=cluster_chyt,
        alias=alias,
        token=token_chyt,
        columns=[],
        create_table_dict={
            'path': path,
            'tables_dir': tables_cube_tmp_dir,
            'cluster': cluster
        }
    )
    print("Done for {0}".format(
        '//home/{0}/cooking_cubes/acquisition_cube/without_history_part/sources/tmp/sku_dict'.format(folder)))

    query = '''
    CREATE TABLE "//home/{1}/cooking_cubes/acquisition_cube/without_history_part/sources/tmp/ba_puid_dict" ENGINE = YtTable() AS
    SELECT
        t0.*,
        email,
        first_name,
        last_name,
        login,
        phone,
        user_settings_email,
        cloud_status,
        cloud_name,
        cloud_id,
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
            *
        FROM "//home/{1}/cooking_cubes/acquisition_cube/without_history_part/sources/tmp/ba_puid_dict_"
    ) as t0
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
        FROM "//home/{1}/cooking_cubes/acquisition_cube/without_history_part/sources/tmp/clouds"
    ) as t1
    ON t0.puid = t1.puid
    '''.format('', folder)
    path = '//home/{0}/cooking_cubes/acquisition_cube/without_history_part/sources/tmp/ba_puid_dict'.format(folder)
    chyt_execute_query(
        query=query,
        cluster=cluster_chyt,
        alias=alias,
        token=token_chyt,
        columns=[],
        create_table_dict={
            'path': path,
            'tables_dir': tables_cube_tmp_dir,
            'cluster': cluster
        }
    )
    print("Done for {0}".format(
        '//home/{0}/cooking_cubes/acquisition_cube/without_history_part/sources/tmp/ba_puid_dict'.format(folder)))

    query = '''
    CREATE TABLE "//home/{1}/cooking_cubes/acquisition_cube/without_history_part/sources/tmp/bas" ENGINE = YtTable() AS
    SELECT
        billing_account_id,
        'ba_created' as event,
        toString(toDateTime(ba_created_at), 'Etc/UTC') as event_time
    FROM "//home/{1}/cooking_cubes/acquisition_cube/without_history_part/sources/tmp/ba_puid_dict"
    '''.format('', folder)
    path = '//home/{0}/cooking_cubes/acquisition_cube/without_history_part/sources/tmp/bas'.format(folder)
    chyt_execute_query(
        query=query,
        cluster=cluster_chyt,
        alias=alias,
        token=token_chyt,
        columns=[],
        create_table_dict={
            'path': path,
            'tables_dir': tables_cube_tmp_dir,
            'cluster': cluster
        }
    )
    print("Done for {0}".format(
        '//home/{0}/cooking_cubes/acquisition_cube/without_history_part/sources/tmp/bas'.format(folder)))

    puid_tables = [
        paths_dict['visits_path'],
        paths_dict['calls'],
        paths_dict['click_email'],
        '//home/{0}/cooking_cubes/acquisition_cube/without_history_part/sources/tmp/clouds'.format(folder)
    ]
    puid_schema = get_result_schema(puid_tables, cluster)
    query = '''
    CREATE TABLE "//home/{1}/cooking_cubes/acquisition_cube/without_history_part/sources/tmp/puid_set_" ENGINE = YtTable() AS
    {0}
    '''.format(
        'UNION ALL\n'.join(
            make_query_for_union(puid_schema, get_yt_table_schema(table, cluster), table) for table in puid_tables),
        folder
    )
    path = '//home/{0}/cooking_cubes/acquisition_cube/without_history_part/sources/tmp/puid_set_'.format(folder)
    chyt_execute_query(
        query=query,
        cluster=cluster_chyt,
        alias=alias,
        token=token_chyt,
        columns=[],
        create_table_dict={
            'path': path,
            'tables_dir': tables_cube_tmp_dir,
            'cluster': cluster
        }
    )
    print("Done for {0}".format(
        '//home/{0}/cooking_cubes/acquisition_cube/without_history_part/sources/tmp/puid_set_'.format(folder)))

    query = '''
    CREATE TABLE "//home/{1}/cooking_cubes/acquisition_cube/without_history_part/sources/tmp/puid_set" ENGINE = YtTable() AS
    SELECT
        t0.*,
        billing_account_id,
        ba_created_at,
        ba_name,
        ba_state,
        ba_person_type,
        ba_payment_cycle_type,
        ba_usage_status,
        ba_currency,
        ba_type,
        block_reason,
        unblock_reason,
        balance,
        is_verified,
        promocode_client_name,
        promocode_type,
        promocode_source,
        promocode_client_type,
        segment,
        architect,
        is_iot,
        is_isv,
        is_var,
        multiIf(
            isNotNull(account_name) AND (account_name != 'unknown'), account_name,
            isNotNull(master_account_id) AND (master_account_id != ''), ba_name,
            isNotNull(balance_name), balance_name,
            isNotNull(ba_name), ba_name,
            CONCAT(first_name, ' ', last_name)
        ) AS account_name,
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
        active_grants,
        balance_name,
        balance_long_name
    FROM(
        SELECT
            *
        FROM "//home/{1}/cooking_cubes/acquisition_cube/without_history_part/sources/tmp/puid_set_"
    ) as t0
    ANY LEFT JOIN(
        SELECT
            *
        FROM "//home/{1}/cooking_cubes/acquisition_cube/without_history_part/sources/tmp/ba_puid_dict_"
    ) as t1
    ON t0.puid = t1.puid
    '''.format(
        '',
        folder
    )
    path = '//home/{0}/cooking_cubes/acquisition_cube/without_history_part/sources/tmp/puid_set'.format(folder)
    chyt_execute_query(
        query=query,
        cluster=cluster_chyt,
        alias=alias,
        token=token_chyt,
        columns=[],
        create_table_dict={
            'path': path,
            'tables_dir': tables_cube_tmp_dir,
            'cluster': cluster
        }
    )
    print("Done for {0}".format(
        '//home/{0}/cooking_cubes/acquisition_cube/without_history_part/sources/tmp/puid_set'.format(folder)))

    query = '''
    CREATE TABLE "//home/{1}/cooking_cubes/acquisition_cube/without_history_part/sources/tmp/ba_became_paid" ENGINE = YtTable() AS
    SELECT
        billing_account_id,
        toString(MIN(toDateTime(updated_at)), 'Etc/UTC') as event_time,
        'ba_became_paid' as event
    FROM "{0}"
    WHERE
        lower(usage_status) = 'paid'
    GROUP BY
        billing_account_id,
        event
    '''.format(paths_dict['billing_accounts_history_path'], folder)
    path = '//home/{0}/cooking_cubes/acquisition_cube/without_history_part/sources/tmp/ba_became_paid'.format(folder)
    chyt_execute_query(
        query=query,
        cluster=cluster_chyt,
        alias=alias,
        token=token_chyt,
        columns=[],
        create_table_dict={
            'path': path,
            'tables_dir': tables_cube_tmp_dir,
            'cluster': cluster
        }
    )
    print("Done for {0}".format(
        '//home/{0}/cooking_cubes/acquisition_cube/without_history_part/sources/tmp/ba_became_paid'.format(folder)))

    query = '''
    CREATE TABLE "//home/{1}/cooking_cubes/acquisition_cube/without_history_part/sources/tmp/first_trial_consumption" ENGINE = YtTable() AS
    SELECT
        billing_account_id,
        'first_trial_consumption' as event,
        toString(toDateTime(argMin(end_time, end_time), 'Etc/UTC')) as event_time,
        argMin(sku_id, end_time) as sku_id
    FROM "{0}"
    WHERE
        toFloat64(credit) < 0
    GROUP BY
        billing_account_id,
        event
    '''.format(paths_dict['billing_records_path'], folder)
    path = '//home/{0}/cooking_cubes/acquisition_cube/without_history_part/sources/tmp/first_trial_consumption'.format(
        folder)
    chyt_execute_query(
        query=query,
        cluster=cluster_chyt,
        alias=alias,
        token=token_chyt,
        columns=[],
        create_table_dict={
            'path': path,
            'tables_dir': tables_cube_tmp_dir,
            'cluster': cluster
        }
    )
    print("Done for {0}".format(
        '//home/{0}/cooking_cubes/acquisition_cube/without_history_part/sources/tmp/first_trial_consumption'.format(
            folder)))

    query = '''
    CREATE TABLE "//home/{1}/cooking_cubes/acquisition_cube/without_history_part/sources/tmp/first_paid_consumption" ENGINE = YtTable() AS
    SELECT
        billing_account_id,
        'first_paid_consumption' as event,
        toString(toDateTime(argMin(end_time, end_time), 'Etc/UTC')) as event_time,
        argMin(sku_id, end_time) as sku_id
    FROM "{0}"
    WHERE
        toFloat64(credit) + toFloat64(cost) > 0
    GROUP BY
        billing_account_id,
        event
    '''.format(paths_dict['billing_records_path'], folder)
    path = '//home/{0}/cooking_cubes/acquisition_cube/without_history_part/sources/tmp/first_paid_consumption'.format(
        folder)
    chyt_execute_query(
        query=query,
        cluster=cluster_chyt,
        alias=alias,
        token=token_chyt,
        columns=[],
        create_table_dict={
            'path': path,
            'tables_dir': tables_cube_tmp_dir,
            'cluster': cluster
        }
    )
    print("Done for {0}".format(
        '//home/{0}/cooking_cubes/acquisition_cube/without_history_part/sources/tmp/first_paid_consumption'.format(
            folder)))

    query = '''
    CREATE TABLE "//home/{1}/cooking_cubes/acquisition_cube/without_history_part/sources/tmp/first_payment" ENGINE = YtTable() AS
    SELECT
        billing_account_id,
        'first_payment' as event,
        argMin(toString(toDateTime(modified_at), 'Etc/UTC'), modified_at) as event_time,
        argMin(toFloat64(amount), modified_at) as amount,
        argMin(replaceRegexpAll(extract(context, '"payment_type": "[a-zA-Z _]+"'), '("payment_type": "|")', ''), modified_at) as payment_type
    FROM "{0}"
    WHERE
        type = 'payments'
        AND status = 'ok'
    GROUP BY
        billing_account_id,
        event
    '''.format(paths_dict['transactions_path'], folder)
    path = '//home/{0}/cooking_cubes/acquisition_cube/without_history_part/sources/tmp/first_payment'.format(folder)
    chyt_execute_query(
        query=query,
        cluster=cluster_chyt,
        alias=alias,
        token=token_chyt,
        columns=[],
        create_table_dict={
            'path': path,
            'tables_dir': tables_cube_tmp_dir,
            'cluster': cluster
        }
    )
    print("Done for {0}".format(
        '//home/{0}/cooking_cubes/acquisition_cube/without_history_part/sources/tmp/first_payment'.format(folder)))

    ba_tables = [
        '//home/{0}/cooking_cubes/acquisition_cube/without_history_part/sources/tmp/bas'.format(folder),
        '//home/{0}/cooking_cubes/acquisition_cube/without_history_part/sources/tmp/first_paid_consumption'.format(
            folder),
        '//home/{0}/cooking_cubes/acquisition_cube/without_history_part/sources/tmp/first_payment'.format(folder),
        '//home/{0}/cooking_cubes/acquisition_cube/without_history_part/sources/tmp/first_trial_consumption'.format(
            folder),
        '//home/{0}/cooking_cubes/acquisition_cube/without_history_part/sources/tmp/ba_became_paid'.format(folder)
    ]
    ba_schema = get_result_schema(ba_tables, cluster)
    query = '''
    CREATE TABLE "//home/{1}/cooking_cubes/acquisition_cube/without_history_part/sources/tmp/ba_set_" ENGINE = YtTable() AS
    {0}
    '''.format(
        'UNION ALL\n'.join(
            make_query_for_union(ba_schema, get_yt_table_schema(table, cluster), table) for table in ba_tables),
        folder
    )
    path = '//home/{0}/cooking_cubes/acquisition_cube/without_history_part/sources/tmp/ba_set_'.format(folder)
    chyt_execute_query(
        query=query,
        cluster=cluster_chyt,
        alias=alias,
        token=token_chyt,
        columns=[],
        create_table_dict={
            'path': path,
            'tables_dir': tables_cube_tmp_dir,
            'cluster': cluster
        }
    )
    print("Done for {0}".format(
        '//home/{0}/cooking_cubes/acquisition_cube/without_history_part/sources/tmp/ba_set_'.format(folder)))

    query = '''
    CREATE TABLE "//home/{1}/cooking_cubes/acquisition_cube/without_history_part/sources/tmp/ba_set" ENGINE = YtTable() AS
    SELECT
        ba_ba_dict.*,
        name,
        service_id,
        service_long_name,
        subservice_name,
        platform,
        service_name,
        service_group,
        core_fraction,
        database,
        sku_name,
        sku_lazy,
        preemptible
    FROM(
        SELECT
            ba.*,
            billing_account_id,
            ba_created_at,
            ba_name,
            ba_state,
            ba_person_type,
            ba_payment_cycle_type,
            ba_usage_status,
            ba_currency,
            ba_type,
            puid,
            block_reason,
            unblock_reason,
            balance,
            is_verified,
            promocode_client_name,
            promocode_type,
            promocode_source,
            promocode_client_type,
            segment,
            architect,
            is_iot,
            is_isv,
            is_var,
            multiIf(
                isNotNull(account_name) AND (account_name != 'unknown'), account_name,
                isNotNull(master_account_id) AND (master_account_id != ''), ba_name,
                isNotNull(balance_name), balance_name,
                isNotNull(ba_name), ba_name,
                CONCAT(first_name, ' ', last_name)
            ) AS account_name,
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
            active_grants,
            balance_name,
            balance_long_name,
            email,
            first_name,
            last_name,
            login,
            phone,
            user_settings_email,
            cloud_status,
            cloud_name,
            cloud_id,
            mail_tech,
            mail_testing,
            mail_info,
            mail_feature,
            mail_event,
            mail_billing,
            mail_promo,
            is_corporate_card
        FROM(
            SELECT
                *
            FROM "//home/{1}/cooking_cubes/acquisition_cube/without_history_part/sources/tmp/ba_set_"
        ) as ba
        ANY LEFT JOIN(
            SELECT
                *
            FROM "//home/{1}/cooking_cubes/acquisition_cube/without_history_part/sources/tmp/ba_puid_dict"
        ) as ba_dict
        ON ba.billing_account_id = ba_dict.billing_account_id
    ) as ba_ba_dict
    ANY LEFT JOIN(
        SELECT
            *
        FROM "//home/{1}/cooking_cubes/acquisition_cube/without_history_part/sources/tmp/sku_dict"
    ) as sku
    ON ba_ba_dict.sku_id = sku.sku_id
    '''.format('', folder, paths_dict['billing_accounts_path'])
    path = '//home/{0}/cooking_cubes/acquisition_cube/without_history_part/sources/tmp/ba_set'.format(folder)
    chyt_execute_query(
        query=query,
        cluster=cluster_chyt,
        alias=alias,
        token=token_chyt,
        columns=[],
        create_table_dict={
            'path': path,
            'tables_dir': tables_cube_tmp_dir,
            'cluster': cluster
        }
    )
    print("Done for {0}".format(
        '//home/{0}/cooking_cubes/acquisition_cube/without_history_part/sources/tmp/ba_set'.format(folder)))

    from acquisition_cube_init_funnel_steps import schema_cube as schema
    job = cluster.job()
    job.concat(
        job.table('//home/{0}/cooking_cubes/acquisition_cube/without_history_part/sources/tmp/puid_set'.format(folder)),
        job.table('//home/{0}/cooking_cubes/acquisition_cube/without_history_part/sources/tmp/ba_set'.format(folder))
    ) \
        .project(
        **apply_types_in_project(schema)
    ) \
        .put('%s/%s_temp' % (paths_dict['cube_tmp'], str(datetime.datetime.today().date())), schema=schema)
    job.run()

    job = cluster.job()
    cube = job.table('%s/%s_temp' % (paths_dict['cube_tmp'], str(datetime.datetime.today().date()))) \
        .groupby(
        'puid'
    ) \
        .sort(
        'event_time'
    ) \
        .reduce(
        apply_attribution_reduce,
        memory_limit=8192
    ) \
        .project(
        **apply_types_in_project(schema)
    )
    cube \
        .put('%s/%s_to_validate' % (paths_dict['funnel_cube'], str(datetime.datetime.today().date())), schema=schema)
    job.run()

    schema = {
        'person_last_name': str,
        'sku_tag_service_name': str,
        'usd_rur_quote': float,
        'person_mail_feature': int,
        'billing_account_company_name': str,
        'sku_tag_lazy': int,
        'billing_account_segment_at_moment': str,
        'event_time': str,
        'person_user_settings_email': str,
        'metrika_resolution_depth': int,
        'sku_tag_platform': str,
        'metrika_ad_block': int,
        'person_mail_testing': int,
        'metrika_general_interests': str,
        'metrika_utm_source': str,
        'person_cloud_name': str,
        'billing_account_currency': str,
        'billing_account_state_at_moment': str,
        'sku_tag_core_fraction': str,
        'person_mail_billing': int,
        'billing_account_block_reason_at_moment': str,
        'billing_account_unblock_reason_at_moment': str,
        'person_mail_event': int,
        'metrika_sex': str,
        'metrika_device_type': str,
        'billing_account_name': str,
        'billing_account_active_grants_at_moment': int,
        'event': str,
        'person_first_name': str,
        'billing_record_cloud_id': str,
        'billing_account_board_segment_at_moment': str,
        'billing_record_pricing_quantity': float,
        'billing_account_grant_start_times_at_moment': str,
        'metrika_utm_medium': str,
        'billing_account_person_type_at_moment': str,
        'person_email': str,
        'person_cloud_status': str,
        'person_mail_tech': int,
        'metrika_city': str,
        'billing_record_pricing_unit': str,
        'billing_account_usage_status_at_moment': str,
        'billing_account_usage_status': str,
        'billing_account_usage_status_actual': str,
        'metrika_hits': int,
        'billing_account_master_account_id': str,
        'billing_account_grant_end_times_at_moment': str,
        'metrika_resolution_height': int,
        'billing_account_is_var_at_moment': str,
        'metrika_search_phrase': str,
        'metrika_start_url': str,
        'person_login': str,
        'metrika_os': str,
        'metrika_is_robot': str,
        'sku_id': str,
        'metrika_utm_content': str,
        'billing_account_grant_sources_at_moment': str,
        'metrika_resolution_width': int,
        'total': str,
        'metrika_utm_campaign': str,
        'metrika_area': str,
        'billing_account_id': str,
        'billing_account_is_corporate_card': int,
        'billing_account_is_verified_at_moment': str,
        'metrika_total_visits': int,
        'metrika_interests': str,
        'metrika_client_ip': str,
        'sku_name': str,
        'person_phone': str,
        'metrika_referer': str,
        'metrika_mobile_phone_vendor': int,
        'metrika_session_start_time': str,
        'billing_account_grant_amounts_at_moment': str,
        'billing_account_is_iot_at_moment': int,
        'metrika_is_bounce': int,
        'metrika_device_model': str,
        'billing_account_architect_at_moment': str,
        'metrika_country': str,
        'billing_account_sales_name_at_moment': str,
        'service_id': str,
        'billing_account_balance_at_moment': float,
        'sku_tag_subservice_name': str,
        'sku_tag_service_long_name': str,
        'metrika_duration': int,
        'billing_account_balance_name_at_moment': str,
        'billing_account_active_grant_ids_at_moment': str,
        'sku_tag_service_group': str,
        'metrika_utm_term': str,
        'billing_account_is_isv_at_moment': int,
        'billing_account_is_isv': int,
        'puid': str,
        'billing_account_potential_at_moment': str,
        'metrika_income': int,
        'metrika_page_views': int,
        'billing_account_grant_source_ids_at_moment': str,
        'metrika_channel': str,
        'metrika_remote_ip': str,
        'billing_account_balance_long_name_at_moment': str,
        'billing_account_payment_cycle_type_at_moment': str,
        'sku_tag_preemptible': str,
        'billing_account_type_at_moment': str,
        'metrika_window_client_height': int,
        'sku_tag_database': str,
        'billing_record_usage_unit': str,
        'metrika_age': str,
        'billing_account_is_fraud': int,
        'person_mail_info': int,
        'billing_account_grant_amount_at_moment': float,
        'person_mail_promo': int,
        'billing_account_architect': str,
        'billing_account_balance': str,
        'billing_account_board_segment': str,
        'billing_account_creation_date': str,
        'billing_account_creation_month': str,
        'billing_account_creation_time': str,
        'billing_account_creation_week': str,
        'billing_account_is_iot': int,
        'billing_account_is_var': str,
        'billing_account_is_verified': str,
        'billing_account_metadata': str,
        'billing_account_payment_cycle_type': str,
        'billing_account_payment_method_id': str,
        'billing_account_person_type': str,
        'billing_account_potential': str,
        'billing_account_sales_name': str,
        'billing_account_segment': str,
        'billing_account_state': str,
        'billing_account_type': str,
        'billing_record_committed_use_discount_credit_charge': float,
        'billing_record_committed_use_discount_credit_charge_rub': float,
        'billing_record_cost': float,
        'billing_record_cost_rub': float,
        'billing_record_created_at': int,
        'billing_record_credit': float,
        'billing_record_credit_charges': float,
        'billing_record_date': str,
        'billing_record_disabled_credit_charge': float,
        'billing_record_disabled_credit_charge_rub': float,
        'billing_record_end_time': int,
        'billing_record_monetary_grant_credit_charge': float,
        'billing_record_monetary_grant_credit_charge_rub': float,
        'billing_record_month': str,
        'billing_record_service_credit_charge': float,
        'billing_record_service_credit_charge_rub': float,
        'billing_record_start_time': int,
        'billing_record_total': float,
        'billing_record_total_rub': float,
        'billing_record_trial_credit_charge': float,
        'billing_record_trial_credit_charge_rub': float,
        'billing_record_volume_incentive_credit_charge': float,
        'billing_record_volume_incentive_credit_charge_rub': float,
        'cb_quote': float,
        'labels_folder_id': str,
        'labels_hash': int,
        'labels_json': str,
        'labels_system_labels': str,
        'labels_user_labels': str,
        'product_name': str,
        'service_name': str,
        'sku_tag_core_fraction_number': float,
        'sku_tag_cores': str,
        'sku_tag_cores_number': float,
        'sku_tag_ram': str,
        'sku_tag_ram_number': float,
        'billing_account_created_at': int,
        'billing_record_credit_rub': float,
        'billing_account_puid': str
    }

    query = '''
    CREATE TABLE "{0}" ENGINE = YtTable() AS
    SELECT
        last_name as person_last_name,
        service_name as sku_tag_service_name,
        usd_rur_quote as usd_rur_quote,
        mail_feature as person_mail_feature,
        account_name as billing_account_company_name,
        sku_lazy as sku_tag_lazy,
        segment as billing_account_segment_at_moment,
        event_time as event_time,
        user_settings_email as person_user_settings_email,
        resolution_depth as metrika_resolution_depth,
        platform as sku_tag_platform,
        ad_block as metrika_ad_block,
        mail_testing as person_mail_testing,
        general_interests as metrika_general_interests,
        utm_source as metrika_utm_source,
        cloud_name as person_cloud_name,
        ba_currency as billing_account_currency,
        ba_state as billing_account_state_at_moment,
        core_fraction as sku_tag_core_fraction,
        mail_billing as person_mail_billing,
        block_reason as billing_account_block_reason_at_moment,
        unblock_reason as billing_account_unblock_reason_at_moment,
        mail_event as person_mail_event,
        sex as metrika_sex,
        device_type as metrika_device_type,
        ba_name as billing_account_name,
        active_grants as billing_account_active_grants_at_moment,
        event as event,
        first_name as person_first_name,
        cloud_id as billing_record_cloud_id,
        board_segment as billing_account_board_segment_at_moment,
        pricing_quantity as billing_record_pricing_quantity,
        grant_start_times as billing_account_grant_start_times_at_moment,
        utm_medium as metrika_utm_medium,
        ba_person_type as billing_account_person_type_at_moment,
        email as person_email,
        cloud_status as person_cloud_status,
        mail_tech as person_mail_tech,
        city as metrika_city,
        pricing_unit as billing_record_pricing_unit,
        ba_usage_status as billing_account_usage_status_at_moment,
        ba_usage_status as billing_account_usage_status,
        ba_usage_status as billing_account_usage_status_actual,
        hits as metrika_hits,
        master_account_id as billing_account_master_account_id,
        grant_end_times as billing_account_grant_end_times_at_moment,
        resolution_height as metrika_resolution_height,
        is_var as billing_account_is_var_at_moment,
        search_phrase as metrika_search_phrase,
        start_url as metrika_start_url,
        login as person_login,
        os as metrika_os,
        is_robot as metrika_is_robot,
        sku_id as sku_id,
        utm_content as metrika_utm_content,
        grant_sources as billing_account_grant_sources_at_moment,
        resolution_width as metrika_resolution_width,
        total as total,
        utm_campaign as metrika_utm_campaign,
        area as metrika_area,
        billing_account_id as billing_account_id,
        is_corporate_card as billing_account_is_corporate_card,
        is_verified as billing_account_is_verified_at_moment,
        total_visits as metrika_total_visits,
        interests as metrika_interests,
        client_ip as metrika_client_ip,
        sku_name as sku_name,
        phone as person_phone,
        referer as metrika_referer,
        mobile_phone_vendor as metrika_mobile_phone_vendor,
        session_start_time as metrika_session_start_time,
        grant_amounts as billing_account_grant_amounts_at_moment,
        is_iot as billing_account_is_iot_at_moment,
        is_bounce as metrika_is_bounce,
        device_model as metrika_device_model,
        architect billing_account_architect_at_moment,
        country as metrika_country,
        sales_name as billing_account_sales_name_at_moment,
        service_id as service_id,
        balance as billing_account_balance_at_moment,
        subservice_name as sku_tag_subservice_name,
        service_long_name as sku_tag_service_long_name,
        duration as metrika_duration,
        balance_name as billing_account_balance_name_at_moment,
        active_grant_ids as billing_account_active_grant_ids_at_moment,
        service_group as sku_tag_service_group,
        utm_term as metrika_utm_term,
        is_isv as billing_account_is_isv_at_moment,
        is_isv as billing_account_is_isv,
        puid as puid,
        potential as billing_account_potential_at_moment,
        income as metrika_income,
        page_views as metrika_page_views,
        grant_source_ids as billing_account_grant_source_ids_at_moment,
        channel as metrika_channel,
        remote_ip as metrika_remote_ip,
        balance_long_name as billing_account_balance_long_name_at_moment,
        ba_payment_cycle_type as billing_account_payment_cycle_type_at_moment,
        preemptible as sku_tag_preemptible,
        ba_type as billing_account_type_at_moment,
        window_client_height as metrika_window_client_height,
        database as sku_tag_database,
        usage_unit as billing_record_usage_unit,
        age as metrika_age,
        window_client_width as metrika_window_client_width,
        is_fraud as billing_account_is_fraud,
        mail_info as person_mail_info,
        grant_amount as billing_account_grant_amount_at_moment,
        mail_promo as person_mail_promo,
        architect as billing_account_architect,
        balance as billing_account_balance,
        board_segment as billing_account_board_segment,
        toString(toDate(ifNull(NULL)(first_ba_created_datetime, '0000-00-00 00:00:00'))) as billing_account_creation_date,
        toString(toStartOfMonth(toDate(ifNull(NULL)(first_ba_created_datetime, '0000-00-00 00:00:00')))) as billing_account_creation_month,
        toString(ifNull(NULL)(first_ba_created_datetime, '0000-00-00 00:00:00')) as billing_account_creation_time,
        toString(toMonday(toDate(ifNull(NULL)(first_ba_created_datetime, '0000-00-00 00:00:00')))) as billing_account_creation_week,
        is_iot as billing_account_is_iot,
        is_var as billing_account_is_var,
        is_verified as billing_account_is_verified,
        '' as billing_account_metadata,
        ba_payment_cycle_type as billing_account_payment_cycle_type,
        '' as billing_account_payment_method_id,
        ba_person_type as billing_account_person_type,
        potential as billing_account_potential,
        sales_name as billing_account_sales_name,
        segment as billing_account_segment,
        ba_state as billing_account_state,
        ba_type as billing_account_type,
        0 as billing_record_committed_use_discount_credit_charge,
        0 as billing_record_committed_use_discount_credit_charge_rub,
        0 as billing_record_cost,
        0 as billing_record_cost_rub,
        0 as billing_record_created_at,
        0 as billing_record_credit,
        0 as billing_record_credit_charges,
        toString(toDate(event_time)) as billing_record_date,
        0 as billing_record_disabled_credit_charge,
        0 as billing_record_disabled_credit_charge_rub,
        toInt64(toDateTime(event_time, 'Etc/UTC')) as billing_record_end_time,
        0 as billing_record_monetary_grant_credit_charge,
        0 as billing_record_monetary_grant_credit_charge_rub,
        toStartOfMonth(toDate(event_time)) as billing_record_month,
        0 as billing_record_service_credit_charge,
        0 as billing_record_service_credit_charge_rub,
        toInt64(toDateTime(event_time, 'Etc/UTC')) as billing_record_start_time,
        0 as billing_account_created_at,
        0 as billing_record_total,
        0 as billing_record_total_rub,
        0 as billing_record_trial_credit_charge,
        0 as billing_record_trial_credit_charge_rub,
        0 as billing_record_volume_incentive_credit_charge,
        0 as billing_record_volume_incentive_credit_charge_rub,
        '' as cb_quote,
        '' as labels_folder_id,
        '' as labels_hash,
        '' as labels_json,
        '' as labels_system_labels,
        '' as labels_user_labels,
        '' as product_name,
        '' as service_name,
        '' as sku_tag_core_fraction_number,
        '' as sku_tag_cores,
        '' as sku_tag_cores_number,
        '' as sku_tag_ram,
        '' as sku_tag_ram_number,
        0.0 as billing_record_credit_rub,
        puid as billing_account_puid
    FROM "{1}"
    '''.format(
        '%s/%s' % (paths_dict['funnel_cube'], 'cube_without_history_part_'),
        '%s/%s_to_validate' % (paths_dict['funnel_cube'], str(datetime.datetime.today().date())),
    )
    path = '%s/%s' % (paths_dict['funnel_cube'], 'cube_without_history_part_')
    chyt_execute_query(
        query=query,
        cluster=cluster_chyt,
        alias=alias,
        token=token_chyt,
        columns=[],
        create_table_dict={
            'path': path,
            'tables_dir': paths_dict['funnel_cube'],
            'cluster': cluster
        }
    )
    print("Done for {0}".format('%s/%s' % (paths_dict['funnel_cube'], 'cube_without_history_part_')))

    job = cluster.job()
    stream = job.table('%s/%s' % (paths_dict['funnel_cube'], 'cube_without_history_part_')) \
        .project(
        **apply_types_in_project(schema)
    )
    stream.put('%s/%s' % (paths_dict['funnel_cube'], 'cube_without_history_part'), schema=schema)
    stream.put('%s/%s' % (paths_dict['funnel_cube'], str(datetime.date.today())), schema=schema)
    job.run()

    tables_to_save = date_range_by_days(str(datetime.date.today() - datetime.timedelta(days=90)),
                                        str(datetime.date.today())) + ['cube_without_history_part',
                                                                       'cube_without_history_part_preprod']
    tables_to_save = [paths_dict['cube_tmp'] + '/' + table for table in tables_to_save]
    for table in get_table_list(paths_dict['cube_tmp'], job).replace('{', '').replace('}', '').split(','):
        if table not in tables_to_save:
            cluster.driver.remove(table)


if __name__ == '__main__':
    main()
