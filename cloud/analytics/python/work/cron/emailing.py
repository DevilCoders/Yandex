#!/usr/bin/env python2
# -*- coding: utf-8 -*-
import pandas as pd, datetime, ast, numpy as np, telebot, json, requests, os, sys, time, re
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
        mail_tech = None
        mail_testing = None
        mail_info = None
        mail_feature = None
        mail_event = None
        mail_promo = None
        mail_billing = None
        for rec in records:
            if rec['mail_tech']:
                mail_tech = rec['mail_tech']
            if rec['mail_testing']:
                mail_testing = rec['mail_testing']
            if rec['mail_info']:
                mail_info = rec['mail_info']
            if rec['mail_feature']:
                mail_feature = rec['mail_feature']
            if rec['mail_event']:
                mail_event = rec['mail_event']
            if rec['mail_promo']:
                mail_promo = rec['mail_promo']
            if rec['mail_billing']:
                mail_billing = rec['mail_billing']
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
        result_dict['mail_tech'] = mail_tech
        result_dict['mail_testing'] = mail_testing
        result_dict['mail_info'] = mail_info
        result_dict['mail_feature'] = mail_feature
        result_dict['mail_event'] = mail_event
        result_dict['mail_promo'] = mail_promo
        result_dict['mail_billing'] = mail_billing
        yield Record(key, **result_dict)

def get_mail_id(mail_desc):
    mail_desc = mail_desc.lower()
    if '|' in mail_desc:
        if re.match('^\d+\.\d+\.\d+\|.*', mail_desc):
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

def get_table_list(folder_path, job):
    tables_list = sorted([folder_path + '/' + x for x in job.driver.list(folder_path)], reverse=True)
    return '{%s}' % (','.join(tables_list))

def main():
    cluster_chyt = 'hahn'
    alias = "*cloud_analytics"
    token_chyt = '%s' % (yt_creds['value']['token'])
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
    send_email = job.table(paths_dict['send_email']) \
    .unique(
        'marketo_id'
    ) \
    .project(
        event = ne.const('email_sended'),
        event_time = ne.custom(lambda x: get_datetime_from_epoch(x), 'created'),
        email = ne.custom(works_with_emails,'email'),
        mail_id = ne.custom(get_mail_id, 'mailing_name'),
        mailing_name = 'mailing_name'

    ) \
    .put(
        '//home/cloud_analytics_test/cubes/emailing/send_email',
        schema = {'event': str, 'event_time': str, 'email': str, 'mail_id': str,'mailing_name': str }
    )



    email_delivered = job.table(paths_dict['email_delivered']) \
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
    ) \
    .put(
        '//home/cloud_analytics_test/cubes/emailing/email_delivered',
        schema = {'email': str, 'mail_id': str, 'mailing_name': str, 'delivery_time': str}
    )

    open_email = job.table(paths_dict['open_email']) \
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
    ) \
    .put(
        '//home/cloud_analytics_test/cubes/emailing/open_email',
        schema = {'email': str, 'mail_id': str, 'mailing_name': str, 'open_time': str}
    )

    click_email = job.table(paths_dict['click_email']) \
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
    ) \
    .put(
        '//home/cloud_analytics_test/cubes/emailing/click_email',
        schema = {'email': str, 'mail_id': str, 'mailing_name': str, 'click_time': str}
    )
    job.run()

    schema = {
        "ba_became_paid_time": str,
        "ba_created_time": str,
        "ba_state": str,
        "billing_account_id": str,
        "block_reason": str,
        "cloud_created_time": str,
        "email": str,
        "first_paid_consumption_time": str,
        "first_trial_consumption_time": str,
        "is_ba_became_paid": int,
        "is_ba_created": int,
        "is_cloud_created": int,
        "is_first_paid_consumption": int,
        "is_first_trial_consumption": int,
        "mail_billing": int,
        "mail_event": int,
        "mail_feature": int,
        "mail_info": int,
        "mail_promo": int,
        "mail_tech": int,
        "mail_testing": int,
        "puid": str
    }
    job = cluster.job()
    funnel_events = job.table(paths_dict['acquisition_cube']) \
    .filter(
        nf.custom(lambda x: x not in ['click_mail','call','visit', 'call', 'day_use', 'day_use_payment'])
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
        'email'
    ) \
    .sort(
        'event_time'
    ) \
    .reduce(
        pivot_events
    ) \
    .project(
        **apply_types_in_project(schema)
    ) \
    .put(
        '//home/cloud_analytics_test/cubes/emailing/funnel_events',
        schema = schema
    )
    job.run()

    query = '''
    CREATE TABLE "{0}" ENGINE = YtTable() AS
    SELECT
        sdoc.*,
        is_ba_created,
        ba_became_paid_time,
        mail_billing,
        block_reason,
        mail_event,
        mail_feature,
        ba_created_time,
        billing_account_id,
        is_ba_became_paid,
        puid,
        mail_tech,
        is_first_paid_consumption,
        is_cloud_created,
        mail_testing,
        first_paid_consumption_time,
        is_first_trial_consumption,
        first_trial_consumption_time,
        mail_info,
        ba_state,
        cloud_created_time,
        mail_promo
    FROM(
        SELECT
            sdo.*,
            click.click_time
        FROM(
            SELECT
                sd.*,
                open.open_time
            FROM(
                SELECT
                    send.*,
                    delivedy.delivery_time
                FROM(
                    SELECT
                        *
                    FROM "//home/cloud_analytics_test/cubes/emailing/send_email"
                ) as send
                ANY LEFT JOIN(
                    SELECT
                        *
                    FROM "//home/cloud_analytics_test/cubes/emailing/email_delivered"
                ) as delivedy
                ON send.email = delivedy.email AND send.mail_id = delivedy.mail_id AND send.mailing_name = delivedy.mailing_name
            ) as sd
            ANY LEFT JOIN(
                SELECT
                    *
                FROM "//home/cloud_analytics_test/cubes/emailing/open_email"
            ) as open
            ON sd.email = open.email AND sd.mail_id = open.mail_id AND sd.mailing_name = open.mailing_name
        ) as sdo
        ANY LEFT JOIN(
            SELECT
                *
            FROM "//home/cloud_analytics_test/cubes/emailing/click_email"
        ) as click
        ON sdo.email = click.email AND sdo.mail_id = click.mail_id AND sdo.mailing_name = click.mailing_name
    ) as sdoc
    ANY LEFT JOIN(
        SELECT
            *
        FROM "//home/cloud_analytics_test/cubes/emailing/funnel_events"
    ) as funnel
    ON sdoc.email = funnel.email
    '''.format(paths_dict['emailing_cube']+'_temp')

    path = paths_dict['emailing_cube']+'_temp'

    chyt_execute_query(
        query=query,
        cluster=cluster_chyt,
        alias=alias,
        token=token_chyt,
        columns = [],
        create_table_dict = {
            'path': path,
            'tables_dir': '//home/cloud_analytics_test/cubes/emailing',
            'cluster': cluster
        }
    )
    print("Done for {0}".format(paths_dict['emailing_cube']+'_temp'))

    query = '''
    CREATE TABLE "{0}" ENGINE = YtTable() AS
    SELECT
        is_ba_created,
        ba_became_paid_time,
        block_reason,
        mail_id,
        toInt64(is_paid_more_then_10_rur) as is_paid_more_then_10_rur,
        toInt64(is_trial_more_then_10_rur) as is_trial_more_then_10_rur,
        event,
        ba_created_time,
        event_time,
        billing_account_id,
        is_ba_became_paid,
        puid,
        paid_consumption,
        email,
        is_first_paid_consumption,
        is_cloud_created,
        trial_consumption,
        first_paid_consumption_time,
        is_first_trial_consumption,
        open_time,
        delivery_time,
        click_time,
        first_trial_consumption_time,
        mailing_name,
        ba_state,
        cloud_created_time
    FROM(
        SELECT
            *
        FROM
            "//home/cloud_analytics_test/cubes/emailing/cube_temp"
    ) as t0
    ANY LEFT JOIN(
        SELECT
            billing_account_id,
            SUM(trial_consumption) as trial_consumption,
            multiIf(trial_consumption > 10,1,0) as is_trial_more_then_10_rur,
            SUM(real_consumption) as paid_consumption,
            multiIf(paid_consumption > 10,1,0) as is_paid_more_then_10_rur
        FROM
            "//home/cloud_analytics/cubes/acquisition_cube/cube"
        WHERE
            event = 'day_use'
        GROUP BY
            billing_account_id,
            event_time
    ) as t1
    ON t0.billing_account_id = t1.billing_account_id
    '''.format(paths_dict['emailing_cube']+'_')

    path = paths_dict['emailing_cube']+'_'

    chyt_execute_query(
        query=query,
        cluster=cluster_chyt,
        alias=alias,
        token=token_chyt,
        columns = [],
        create_table_dict = {
            'path': path,
            'tables_dir': '//home/cloud_analytics_test/cubes/emailing',
            'cluster': cluster
        }
    )
    print("Done for {0}".format(paths_dict['emailing_cube']+'_'))


    try:
        cluster.driver.remove(paths_dict['emailing_cube'])
    except:
        pass
    cluster.driver.copy(paths_dict['emailing_cube'] + '_', paths_dict['emailing_cube'])
if __name__ == '__main__':
    main()
