#!/usr/bin/env python2
# -*- coding: utf-8 -*-
from acquisition_cube_init_funnel_steps import (
    paths_dict_prod,
    paths_dict_test
)
from creds import (
    yt_creds,
    metrika_creds,
    yc_ch_creds,
    crm_sql_creds,
    stat_creds,
    telebot_creds,
    wiki_creds
)
from nile.api.v1 import (
    clusters,
    aggregators as na,
    extractors as ne,
    filters as nf,
    Record
)
import pandas as pd
import datetime
import ast
import numpy as np
import telebot
import json
import requests
import os
import sys
import time
import re
from datetime import datetime as dt
import json
from functools import partial
module_path = os.path.abspath(os.path.join(
    '/home/ktereshin/yandex/arcadia/cloud/analytics/python/work'))
if module_path not in sys.path:
    sys.path.append(module_path)
module_path = os.path.abspath(os.path.join('/home/ktereshin/yandex/cron'))
if module_path not in sys.path:
    sys.path.append(module_path)


def get_datetime_from_epoch(epoch):
    try:
        return str(datetime.datetime.fromtimestamp(int(epoch)))
    except:
        return None


def apply_types_in_project(schema_):
    apply_types_dict = {}
    for col in schema_:

        if schema_[col] == str:
            apply_types_dict[col] = ne.custom(lambda x: str(x).replace('"', '').replace(
                "'", '').replace('\\', '') if x not in ['', None] else None, col)

        elif schema_[col] == int:
            apply_types_dict[col] = ne.custom(lambda x: int(
                x) if x not in ['', None] else None, col)

        elif schema_[col] == float:
            apply_types_dict[col] = ne.custom(lambda x: float(
                x) if x not in ['', None] else None, col)
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
    url = "{proxy}/query?database={alias}&password={token}".format(
        proxy=proxy, alias=alias, token=token)
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
    create_table_dict={}
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
                    execute_query(query=query, cluster=cluster,
                                  alias=alias, token=token)
                else:
                    execute_query(query=query, cluster=cluster,
                                  alias=alias, token=token)
                return 'Success!!!'
            else:
                result = execute_query(
                    query=query, cluster=cluster, alias=alias, token=token)
                users = pd.DataFrame([row.split('\t')
                                      for row in result], columns=columns)
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
    tables_list = sorted(
        [folder_path + '/' + x for x in job.driver.list(folder_path)], reverse=True)
    return '{%s}' % (','.join(tables_list))


def get_sender_event(event):
    if event == 'send':
        return 'email_sended'
    if event == 'px':
        return 'email_opened'
    if event == 'click':
        return 'email_clicked'
    if event == 'bounce':
        return 'unsub'
    return event


def last_tag(x):
    res = None
    try:
        res = json.loads(x)[-1]
    except Exception:
        pass
    return res


def first_tag(x):
    res = None
    try:
        res = json.loads(x)[0]
    except Exception:
        pass
    return res


def main():
    cluster_chyt = 'hahn'
    alias = "*cloud_analytics"
    token_chyt = '%s' % (yt_creds['value']['token'])
    cluster = clusters.yt.Hahn(
        token=yt_creds['value']['token'],
        pool=yt_creds['value']['pool']
    )

    paths_dict_test = {
        'send_email': '//home/cloud_analytics/import/marketo/send_email',
        'email_delivered': '//home/cloud_analytics/import/marketo/email_delivered',
        'open_email': '//home/cloud_analytics/import/marketo/open_email',
        'click_email': '//home/cloud_analytics/import/marketo/click_email',
        'sender': '//home/cloud_analytics/import/emails/emails_delivery_clicks',
        'acquisition_cube': '//home/cloud_analytics/cubes/acquisition_cube/cube',
        'emailing_cube': '//home/cloud_analytics_test/cubes/emailing_events/cube'
    }
    paths_dict_prod = {
        'send_email': '//home/cloud_analytics/import/marketo/send_email',
        'email_delivered': '//home/cloud_analytics/import/marketo/email_delivered',
        'open_email': '//home/cloud_analytics/import/marketo/open_email',
        'click_email': '//home/cloud_analytics/import/marketo/click_email',
        'sender': '//home/cloud_analytics/import/emails/emails_delivery_clicks',
        'acquisition_cube': '//home/cloud_analytics/cubes/acquisition_cube/cube',
        'emailing_cube': '//home/cloud_analytics/cubes/emailing_events/cube',
    }
    paths_dict = None
    folder = None
    mode = 'prod'
    if mode == 'test':
        paths_dict = paths_dict_test
        folder = 'cloud_analytics_test'
    elif mode == 'prod':
        paths_dict = paths_dict_prod
        folder = 'cloud_analytics'

    job = cluster.job()

    def aggregate_tags(groups):
        for key, records in groups:
            result_dict = {}
            longest_tag_len = 0
            for i, rec in enumerate(records):
                if i == 0:
                    result_dict['program_name'] = rec['tags']
                if (rec['tags'] is not None) and len(rec['tags']) > longest_tag_len:
                    result_dict['mailing_name'] = rec['tags']
            yield Record(key, **result_dict)

    sender = job.table(paths_dict['sender']) \
        .groupby(
        'event',
        'email',
        'unixtime',
        'message_id'
    )\
        .reduce(aggregate_tags)\
        .project(
        event=ne.custom(get_sender_event, 'event'),
        event_time=ne.custom(lambda x: get_datetime_from_epoch(int(
            x)/1000) if int(x) > 1000000000000 else get_datetime_from_epoch(int(x)), 'unixtime'),
        email=ne.custom(works_with_emails, 'email'),
        mail_id='mailing_name',
        mailing_name='mailing_name',
        program_name='program_name',
        stream_name=ne.const('inapplicable'),
        mailing_id=ne.const(0)
    ) \
        .groupby(
        'event',
        'email',
        'mail_id',
        'mailing_name',
        'program_name',
        'stream_name',
        'mailing_id'
    ) \
        .aggregate(
        event_time=na.min('event_time')
    ) \
        .put(
        '//home/{0}/cubes/emailing_events/sender_events'.format(folder),
        schema={'mailing_id': int, 'event': str, 'event_time': str, 'email': str,
                'program_name': str, 'stream_name': str, 'mail_id': str, 'mailing_name': str}
    )

    send_email = job.table(paths_dict['send_email']) \
        .project(
        event=ne.const('email_sended'),
        event_time=ne.custom(lambda x: get_datetime_from_epoch(x), 'created'),
        email=ne.custom(works_with_emails, 'email'),
        mail_id=ne.custom(get_mail_id, 'mailing_name'),
        mailing_name='mailing_name',
        program_name=ne.const('inapplicable'),
        stream_name=ne.const('inapplicable'),
        mailing_id='mailing_id'
    ) \
        .put(
        '//home/{0}/cubes/emailing_events/send_email'.format(folder),
        schema={'mailing_id': int, 'event': str, 'event_time': str, 'email': str,
                'program_name': str, 'stream_name': str, 'mail_id': str, 'mailing_name': str}
    )

    email_delivered = job.table(paths_dict['email_delivered']) \
        .project(
        event=ne.const('email_deliveried'),
        event_time=ne.custom(lambda x: get_datetime_from_epoch(x), 'created'),
        email=ne.custom(works_with_emails, 'email'),
        mail_id=ne.custom(get_mail_id, 'mailing_name'),
        mailing_name='mailing_name',
        program_name=ne.const('inapplicable'),
        stream_name=ne.const('inapplicable'),
        mailing_id='mailing_id'

    ) \
        .groupby(
        'event',
        'email',
        'mail_id',
        'mailing_name',
        'program_name',
        'stream_name',
        'mailing_id'
    ) \
        .aggregate(
        event_time=na.min('event_time')
    ) \
        .put(
        '//home/{0}/cubes/emailing_events/email_delivered'.format(folder),
        schema={'mailing_id': int, 'event': str, 'event_time': str, 'email': str,
                'program_name': str, 'stream_name': str, 'mail_id': str, 'mailing_name': str}
    )

    open_email = job.table(paths_dict['open_email']) \
        .project(
        event=ne.const('email_opened'),
        event_time=ne.custom(lambda x: get_datetime_from_epoch(x), 'created'),
        email=ne.custom(works_with_emails, 'email'),
        mail_id=ne.custom(get_mail_id, 'mailing_name'),
        mailing_name='mailing_name',
        program_name=ne.const('inapplicable'),
        stream_name=ne.const('inapplicable'),
        mailing_id='mailing_id'

    ) \
        .groupby(
        'event',
        'email',
        'mail_id',
        'mailing_name',
        'program_name',
        'stream_name',
        'mailing_id'
    ) \
        .aggregate(
        event_time=na.min('event_time')
    ) \
        .put(
        '//home/{0}/cubes/emailing_events/open_email'.format(folder),
        schema={'mailing_id': int, 'event': str, 'event_time': str, 'email': str,
                'program_name': str, 'stream_name': str, 'mail_id': str, 'mailing_name': str}
    )

    click_email = job.table(paths_dict['click_email']) \
        .unique(
        'marketo_id'
    ) \
        .project(
        event=ne.const('email_clicked'),
        event_time=ne.custom(lambda x: get_datetime_from_epoch(x), 'created'),
        email=ne.custom(works_with_emails, 'email'),
        mail_id=ne.custom(get_mail_id, 'mailing_name'),
        mailing_name='mailing_name',
        program_name=ne.const('inapplicable'),
        stream_name=ne.const('inapplicable'),
        mailing_id='mailing_id'
    ) \
        .groupby(
        'event',
        'email',
        'mail_id',
        'mailing_name',
        'program_name',
        'stream_name',
        'mailing_id'
    ) \
        .aggregate(
        event_time=na.min('event_time')
    ) \
        .put(
        '//home/{0}/cubes/emailing_events/click_email'.format(folder),
        schema={'mailing_id': int, 'event': str, 'event_time': str, 'email': str,
                'program_name': str, 'stream_name': str, 'mail_id': str, 'mailing_name': str}
    )
    job.concat(
        send_email,
        email_delivered,
        open_email,
        click_email,
        sender
    ) \
        .put(
        '//home/{0}/cubes/emailing_events/email_events'.format(folder),
        schema={'mailing_id': int, 'event': str, 'event_time': str, 'email': str,
                'program_name': str, 'stream_name': str, 'mail_id': str, 'mailing_name': str}
    )
    job.run()

    def parse_wiki_table(table_path='users/ktereshin/ProgramId-StreamId-Dictionary/'):
        headers = {
            'Authorization': "OAuth %s" % wiki_creds['value']['token']
        }

        url = 'https://wiki-api.yandex-team.ru/_api/frontend/%s/.grid' % table_path

        r = requests.get(url, headers=headers)
        j = r.json()

        columns = [f['title'] for f in j['data']['structure']['fields']]
        rows = [[c['raw'] for c in row] for row in j['data']['rows']]
        return columns, rows

    def get_track_ids():
        columns, rows = parse_wiki_table(
            table_path='users/ktereshin/ProgramId-StreamId-Dictionary/')
        track_ids = {}
        for list_ in rows:
            if list_[3] != '':
                if int(list_[2]) not in track_ids:
                    track_ids[int(list_[2])] = {}
                else:
                    pass
                track_ids[int(list_[2])][int(list_[3])] = {
                    'program_name': list_[0], 'stream_name': list_[1]}
        return track_ids

    def sender_nurture_tags_table():
        columns, rows = parse_wiki_table(
            table_path='users/oleus/nurture-programs-cloud/')
        sender_nurture = pd.DataFrame(rows, columns=columns)
        return sender_nurture

    track_ids = get_track_ids()

    def get_program_name(track_ids, program_id, stream_id):
        if program_id in track_ids:
            if stream_id in track_ids[program_id]:
                return track_ids[program_id][stream_id]['program_name']
        return 'unknown'

    def get_stream_name(track_ids, program_id, stream_id):
        if program_id in track_ids:
            if stream_id in track_ids[program_id]:
                return track_ids[program_id][stream_id]['stream_name']
        return 'unknown'
    pget_program_name = partial(get_program_name, track_ids)
    pget_stream_name = partial(get_stream_name, track_ids)

    import yt.wrapper as yt
    import logging

    logger = logging.getLogger(__name__)

    class YTAdapter:
        def __init__(self, token=None, cluster='hahn', pool='cloud_analytics_pool'):
            yt.config["proxy"]["url"] = cluster
            yt.config["token"] = token
            if pool is not None:
                yt.config["pool"] = pool

            if token is None:
                token = os.environ['CH_TOKEN']
            self.yt = yt

        def dict_to_schema(self, column_type_dict):
            return [{"name": col_name, "type": col_type} for col_name, col_type in column_type_dict.items()]

        def save_result(self, result_path, schema, df, append=False):
            table = yt.TablePath(result_path, append=append)
            yt_schema = None
            if isinstance(schema, dict):
                yt_schema = self.dict_to_schema(schema)
            else:
                yt_schema = schema
            if not yt.exists(table):
                yt.create(type='table', path=table,
                          attributes={"schema": yt_schema})
            yt.write_table(table, df.to_dict(orient='records'),
                           format=yt.JsonFormat(attributes={"encode_utf8": False}))
            logger.info("Results are saved")

    sender_nurture_df = sender_nurture_tags_table()
    YTAdapter(token_chyt).save_result('//home/{0}/cubes/emailing_events/wiki_table'.format(folder), df=sender_nurture_df,
                                      schema={'Program': 'string', 'Tags': 'string', 'TableName': 'string'})

    job = cluster.job()

    change_nurture_track = job.table('//home/cloud_analytics/import/marketo/change_nurture_track') \
        .unique(
        'marketo_id'
    ) \
        .project(
        event=ne.const('add_to_nurture_stream'),
        event_time=ne.custom(lambda x: get_datetime_from_epoch(x), 'created'),
        email=ne.custom(works_with_emails, 'email'),
        program_name=ne.custom(
            pget_program_name, 'program_id', 'new_track_id'),
        stream_name=ne.custom(pget_stream_name, 'program_id', 'new_track_id'),
        mail_id=ne.const('inapplicable'),
        mailing_name=ne.const('inapplicable'),
        mailing_id=ne.const(0)

    )
    add_to_nurture = job.table('//home/cloud_analytics/import/marketo/add_to_nurture') \
        .unique(
        'marketo_id'
    ) \
        .project(
        event=ne.const('add_to_nurture_stream'),
        event_time=ne.custom(lambda x: get_datetime_from_epoch(x), 'created'),
        email=ne.custom(works_with_emails, 'email'),
        program_name=ne.custom(pget_program_name, 'program_id', 'track_id'),
        stream_name=ne.custom(pget_stream_name, 'program_id', 'track_id'),
        mail_id=ne.const('inapplicable'),
        mailing_name=ne.const('inapplicable'),
        mailing_id=ne.const(0)

    )

    tables = sender_nurture_df.TableName.unique()
    tables_jobs = [job.table('//home/cloud_analytics/emailing/sender/{table}'.format(table=table))
                   .project(ne.all(),
                            table_name=ne.const(table))
                   for table in tables]

    def capitalize(s):
        return None if s is None else s.capitalize()

    nurture_tables_path = '//home/{0}/cubes/emailing_events/sender_nurture'.format(
        folder)
    nurture_table = job.concat(
        *tables_jobs
    ) \
        .project(email=ne.custom(works_with_emails, 'email'),
                 group=ne.custom(lambda col1, col2: capitalize(col1 if (col1 is not None) else col2),
                                 'group', 'Group'),
                 experiment_date='experiment_date',
                 table_name='table_name') \
        .put(
        nurture_tables_path,
        schema={'email': str, 'group': str,
                'experiment_date': str, 'table_name': str}
    )

    def sender_date(date, stream_name, experiment_date):
        res = None
        if (stream_name is not None) and stream_name.lower() == 'control':
            res = str(dt.strptime(experiment_date, "%Y-%m-%d"))
        else:
            res = date
        return res

    sender_nurture_emails_send = job.table(paths_dict['sender'])\
        .filter(nf.and_(nf.equals('source', 'sender'), nf.equals('event', 'send'))) \
        .project(
            event_time='unixtime',
            email=ne.custom(works_with_emails, 'email'),
            tags='tags'
    )

    def sender_date(date, stream_name, experiment_date):
        from datetime import datetime as dt
        res = None
        if (stream_name is not None) and stream_name.lower() == 'control':
            try:
                res = str(dt.strptime(experiment_date, "%Y-%m-%d"))
            except:
                pass
        else:
            if date is not None:
                res = get_datetime_from_epoch(int(
                    date)/1000) if int(date) > 1000000000000 else get_datetime_from_epoch(int(date))
        return res

    sender_nurture = nurture_table\
        .join(job.table('//home/{0}/cubes/emailing_events/wiki_table'.format(folder)),
              by_left='table_name', by_right='TableName', type='inner')\
        .join(sender_nurture_emails_send,   by_left=['email', 'Tags'], by_right=['email', 'tags'],  type='left')\
        .project(
            event=ne.const('add_to_nurture_stream'),
            event_time=ne.custom(sender_date, 'event_time',
                                 'group', 'experiment_date'),
            email=ne.custom(works_with_emails, 'email'),
            program_name='Program',
            stream_name='group',
            mail_id='Tags',
            mailing_name='Tags',
            mailing_id=ne.const(0)
        )\
        .put(
            '//home/{0}/cubes/emailing_events/sender_events_nurture'.format(
                folder),
            schema={'mailing_id': int, 'event': str, 'event_time': str, 'email': str,
                    'program_name': str, 'stream_name': str, 'mail_id': str, 'mailing_name': str}
        )

    job.concat(
        change_nurture_track,
        add_to_nurture,
        sender_nurture
    ) \
        .put(
        '//home/{0}/cubes/emailing_events/add_to_nurture_stream'.format(
            folder),
        schema={'mailing_id': int, 'event': str, 'event_time': str, 'email': str,
                'program_name': str, 'stream_name': str, 'mail_id': str, 'mailing_name': str}
    )
    job.run()

    query = '''
    CREATE TABLE "{0}" ENGINE = YtTable() AS
    SELECT
        t0.*,
        billing_account_id,
        puid,
        ba_state,
        block_reason,
        is_fraud,
        ba_usage_status,
        segment,
        account_name,
        cloud_created_time,
        ba_created_time,
        first_trial_consumption_time,
        first_paid_consumption_time
    FROM(
        SELECT
            *
        FROM
            "//home/{1}/cubes/emailing_events/email_events"
        UNION ALL
        SELECT
            *
        FROM
            "//home/{1}/cubes/emailing_events/add_to_nurture_stream"
    ) as t0
    ANY LEFT JOIN(
        SELECT
            multiIf(
                user_settings_email LIKE '%@yandex.%' OR user_settings_email LIKE '%@ya.%',CONCAT(lower(replaceAll(splitByString('@',assumeNotNull(user_settings_email))[1], '.', '-')), '@yandex.ru'),
                lower(user_settings_email)
            ) as email,
            billing_account_id,
            puid,
            ba_state,
            multiIf(block_reason IS NULL, 'unlocked', block_reason) as block_reason,
            is_fraud,
            ba_usage_status,
            segment,
            account_name,
            event_time as cloud_created_time,
            first_ba_created_datetime as ba_created_time,
            first_first_trial_consumption_datetime as first_trial_consumption_time,
            first_first_paid_consumption_datetime as first_paid_consumption_time
        FROM
            "//home/cloud_analytics/cubes/acquisition_cube/cube"
        WHERE
            event = 'cloud_created'
            AND master_account_id = ''
            AND billing_account_id != ''
    ) as t1
    ON t0.email = t1.email
    '''.format('//home/{0}/cubes/emailing_events/emailing_events'.format(folder), folder)

    path = '//home/{0}/cubes/emailing_events/emailing_events'.format(folder)

    chyt_execute_query(
        query=query,
        cluster=cluster_chyt,
        alias=alias,
        token=token_chyt,
        columns=[],
        create_table_dict={
            'path': path,
            'tables_dir': '//home/{0}/cubes/emailing_events'.format(folder),
            'cluster': cluster
        }
    )
    print("Done for {0}".format(
        '//home/{0}/cubes/emailing_events/emailing_events'.format(folder)))


if __name__ == '__main__':
    main()
