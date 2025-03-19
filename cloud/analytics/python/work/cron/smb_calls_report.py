#!/usr/bin/env python2
# -*- coding: utf-8 -*-
import pandas as pd, datetime, ast, os,sys, pymysql, time, requests
module_path = os.path.abspath(os.path.join('/home/ktereshin/yandex/arcadia/cloud/analytics/python/work'))
if module_path not in sys.path:
    sys.path.append(module_path)
module_path = os.path.abspath(os.path.join('/home/ktereshin/yandex/cron'))
if module_path not in sys.path:
    sys.path.append(module_path)
from data_loader import clickhouse
from yt.transfer_manager.client import TransferManager
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
    stat_creds,
    telebot_creds
)

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

def exucute_query(query, hosts):
    for host in hosts.split(','):
        cloud_clickhouse_param_dict['host'] = host
        cloud_clickhouse_param_dict['query'] = query
        clickhouse.get_clickhouse_data(**cloud_clickhouse_param_dict)


def get_lead_id(email_lead_id, ba_lead_id, puid_lead_id):
    if ba_lead_id:
        return ba_lead_id
    if puid_lead_id:
        return puid_lead_id
    if email_lead_id:
        return email_lead_id

def exucute_query(query, hosts, cloud_clickhouse_param_dict_):
    for host in hosts.split(','):
        try:
            print('start on host: %s' % (host))
            cloud_clickhouse_param_dict_['host'] = host
            cloud_clickhouse_param_dict_['query'] = query
            clickhouse.get_clickhouse_data(**cloud_clickhouse_param_dict_)
            print('============================\n\n')
        except:
            break


def reduce_func(groups):
    for key, records in groups:
        sales_changes = {}
        options_dict = {
            'ba_payment_cycle_type': 'unknown',
            'ba_person_type': 'unknown',
            'ba_state': 'unknown',
            'ba_usage_status': 'unknown',
            'billing_account_id': None,
            'block_reason': 'unknown',
            'channel': 'unknown',
            'cloud_status': 'unknown',
            'lead_source': 'unknown',
            'lead_source_description': 'unknown',
            'lead_email': None,
            'puid': None
        }
        consumption_trial = {
            'compute': 0,
            'cloud_network': 0,
            'storage': 0,
            'mdb': 0,
            'cloud_ai': 0,
            'nlb': 0,
            'event_time': None
        }
        consumption_paid = {
            'compute': 0,
            'cloud_network': 0,
            'storage': 0,
            'mdb': 0,
            'cloud_ai': 0,
            'nlb': 0,
            'event_time': None
        }
        call_options = [
            'call_description',
            'call_parent_type',
            'lead_phone',
            'call_name',
            'call_status',
            'call_tag'
        ]
        result_list = []
        have_new = 0
        have_assigned = 0
        have_in_progress = 0
        for rec in records:

            if 'lead_state' in rec:
                if rec['lead_state'] == 'New':
                    have_new = 1

                if rec['lead_state'] == 'Assigned':
                    have_assigned = 1

                if rec['lead_state'] == 'In Process':
                    have_in_progress = 1

            if rec['event'] == 'day_use':

                if 'puid' in rec:
                    options_dict['puid'] = rec['puid']

                if 'billing_account_id' in rec:
                    options_dict['billing_account_id'] = rec['billing_account_id']

                for service in consumption_paid:
                    if service == rec['service_name']:
                        consumption_trial[service] += rec['trial_consumption']
                        consumption_paid[service] += rec['real_consumption']
                        consumption_paid['event_time'] = rec['event_time']
                        consumption_trial['event_time'] = rec['event_time']
                        break

            if 'sales_name' in rec and rec['event'] == 'call':
                if rec['sales_name']:
                    sales_changes[rec['event_time']] = rec['sales_name']


            for option in options_dict:
                if option in rec:
                    if rec[option]:
                        options_dict[option] = str(rec[option]).lower()


            if rec['event'] in ['call', 'lead_changed_status', 'first_paid_consumption']:
                result_list.append(rec.to_dict())


        for row in result_list:
            for option in options_dict:
                row[option] = options_dict[option]

            for option in call_options:
                if option not in row:
                    row[option] = None

            if row['event'] == 'first_paid_consumption':
                row['lead_state'] = 'first_paid_consumption'

            for option in consumption_paid:
                if option != 'event_time':
                    row['paid_comsumption_' + option] = None
                    row['trial_comsumption_' + option] = None
            row['paid_comsumption_total'] = None
            row['trial_comsumption_total'] = None

            if sales_changes:
                row['sales_name'] = None
                for time in sorted(sales_changes):
                    if time >= row['event_time']:
                        row['sales_name'] = sales_changes[time]
                        break
                if not row['sales_name']:
                    row['sales_name'] = sales_changes[sorted(sales_changes, reverse=True)[0]]
            else:
                row['sales_name'] = 'unknown'


            if have_new ==0:
                row_ = row.copy()
                row_['lead_state'] = 'New'
                yield Record(key, **row_)
                have_new = 1

            if have_new ==1 and have_assigned == 0 and have_in_progress == 1:
                row_ = row.copy()
                row_['lead_state'] = 'Assigned'
                yield Record(key, **row_)
                have_assigned = 1


            yield Record(key, **row)

        if consumption_paid['event_time']:
            last_row = {
                'event': 'Consumption',
                'event_time': consumption_paid['event_time'],
                'lead_state': 'Paid Consumption',
            }
            for option in options_dict:
                last_row[option] = options_dict[option]

            for option in call_options:
                if option not in last_row:
                    last_row[option] = None


            paid = 0
            trial = 0
            for option in consumption_paid:
                if option != 'event_time':
                    paid += consumption_paid[option]
                    trial += consumption_trial[option]
                    last_row['paid_comsumption_' + option] = consumption_paid[option]
                    last_row['trial_comsumption_' + option] = consumption_trial[option]
            last_row['paid_comsumption_total'] = paid
            last_row['trial_comsumption_total'] = trial

            if sales_changes:
                last_row['sales_name'] = sales_changes[sorted(sales_changes, reverse=True)[0]]
            else:
                last_row['sales_name'] = 'unknown'
            if paid > 0:
                yield Record(key, **last_row)


def main():

    try:
        cluster = clusters.yt.Hahn(
            token = yt_creds['value']['token'],
            pool = yt_creds['value']['pool']
        )
        metrika_clickhouse_param_dict['user'] = metrika_creds['value']['login']
        metrika_clickhouse_param_dict['password'] = metrika_creds['value']['pass']

        cloud_clickhouse_param_dict['user'] = yc_ch_creds['value']['login']
        cloud_clickhouse_param_dict['password'] = yc_ch_creds['value']['pass']

        paths_dict_test = {
            'calls': '//home/cloud_analytics_test/cooking_cubes/crm_leads/calls',
            'leads_history': '//home/cloud_analytics_test/cooking_cubes/crm_leads/leads_history',
            'event_cube': '//home/cloud_analytics/cubes/acquisition_cube/cube',
            'crm_leads_cube': '//home/cloud_analytics_test/cubes/crm_leads/cube'
        }
        paths_dict_prod = {
            'calls': '//home/cloud_analytics/cooking_cubes/crm_leads/calls',
            'leads_history': '//home/cloud_analytics/cooking_cubes/crm_leads/leads_history',
            'event_cube': '//home/cloud_analytics/cubes/acquisition_cube/cube',
            'crm_leads_cube': '//home/cloud_analytics/cubes/crm_leads/cube'
        }

        mode = 'test'
        if mode == 'test':
            paths_dict = paths_dict_test
        elif mode == 'prod':
            paths_dict = paths_dict_prod

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

        cluster.write(paths_dict['calls'], calls_df)

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
            MIN(IFNULL(t2.ba_id,'')) as billing_account_id,
            MIN(l1.lead_source) as lead_source,
            MIN(l1.lead_source_description) as lead_source_description,
            MIN(l2.email_address) as lead_email,
            MIN(l3.user_name) sales_name
        FROM
            leads_audit as sorce
        INNER JOIN  leads l1
            ON l1.id=sorce.parent_id AND l1.deleted = 0
        LEFT JOIN leads_billing_accounts as t1
            ON l1.id = t1.leads_id
        LEFT JOIN billingaccounts as t2
            ON t2.id = t1.billingaccounts_id
        LEFT JOIN  email_addr_bean_rel l2_1
            ON l1.id=l2_1.bean_id AND l2_1.bean_module = 'Leads' AND l2_1.primary_address = 1
        LEFT JOIN  email_addresses l2
            ON l2.id=l2_1.email_address_id
        LEFT JOIN  users l3
            ON sorce.after_value_string=l3.id
        WHERE
            sorce.after_value_string != ''
        GROUP BY
            sorce.parent_id,
            CONCAT('lead_changed_',CAST(sorce.field_name as CHAR(50))),
            sorce.after_value_string
        '''

        leads_history = pd.read_sql_query(query, cnx)
        cnx.close()


        cluster.write(paths_dict['leads_history'], leads_history)

        schema = {
            "lead_id": str,
            "event_time": str,
            "event": str,
            "lead_state": str,
            "ba_payment_cycle_type": str,
            "ba_person_type": str,
            "ba_state": str,
            "ba_usage_status": str,
            "billing_account_id": str,
            "block_reason": str,
            "call_description": str,
            "call_id": str,
            "call_name": str,
            "call_parent_type": str,
            "call_status": str,
            "call_tag": str,
            "channel": str,
            "cloud_status": str,
            "lead_email": str,
            "lead_phone": str,
            "lead_source": str,
            "lead_source_description": str,
            "paid_comsumption_cloud_ai": float,
            "paid_comsumption_cloud_network": float,
            "paid_comsumption_compute": float,
            "paid_comsumption_mdb": float,
            "paid_comsumption_nlb": float,
            "paid_comsumption_storage": float,
            "paid_comsumption_total": float,
            "sales_name": str,
            "segment": str,
            "trial_comsumption_cloud_ai": float,
            "trial_comsumption_cloud_network": float,
            "trial_comsumption_compute": float,
            "trial_comsumption_mdb": float,
            "trial_comsumption_nlb": float,
            "trial_comsumption_storage": float,
            "trial_comsumption_total": float,
            "puid": str,
            "account_name": str
        }

        job = cluster.job()
        leads = job.concat(
            job.table(paths_dict['calls']).project(ne.all(), lead_email = ne.custom( works_with_emails,'lead_email')),
            job.table(paths_dict['leads_history']).project(ne.all(), lead_email = ne.custom( works_with_emails,'lead_email'))
        )
        email_dict = leads \
        .filter(
            nf.custom(lambda x: x not in ['', None], 'lead_email'),
            nf.custom(lambda x: x not in ['', None], 'lead_id')
        ) \
        .unique(
            'lead_email',
            'lead_id'
        ) \
        .project(
            'lead_email',
            email_lead_id = 'lead_id'
        )

        ba_dict = leads \
        .filter(
            nf.custom(lambda x: x not in ['', None], 'billing_account_id'),
            nf.custom(lambda x: x not in ['', None], 'lead_id')
        ) \
        .unique(
            'billing_account_id',
            'lead_id'
        ) \
        .project(
            'billing_account_id',
            ba_lead_id = 'lead_id'
        )

        puid_dict = leads \
        .filter(
            nf.custom(lambda x: x not in ['', None], 'puid'),
            nf.custom(lambda x: x not in ['', None], 'lead_id')
        ) \
        .unique(
            'puid',
            'lead_id'
        ) \
        .project(
            'puid',
            puid_lead_id = 'lead_id'
        )

        event = job.table(paths_dict['event_cube']) \
        .filter(
            nf.custom(lambda x: x in ['day_use', 'first_trial_consumption', 'first_paid_consumption'], 'event'),
            nf.custom(lambda x: x not in [None, ''], 'user_settings_email')
        ) \
        .project(
            'event',
            'trial_consumption',
            'real_consumption',
            'trial_consumption_cum',
            'real_consumption_cum',
            'ba_state',
            'ba_payment_cycle_type',
            'ba_person_type',
            'ba_type',
            'ba_usage_status',
            'billing_account_id',
            'block_reason',
            'channel',
            'cloud_status',
            'name',
            'segment',
            'puid',
            event_time = ne.custom(lambda x: str(datetime.datetime.strptime(x, '%Y-%m-%d %H:%M:%S') + datetime.timedelta(seconds = 10800)), 'event_time'),
            service_name = ne.custom(lambda x: x.split('.')[0] if '.' in str(x) else x, 'service_name'),
            lead_email = ne.custom( works_with_emails,'user_settings_email')
        ) \
        .join(
            email_dict,
            by = 'lead_email',
            type = 'left'
        ) \
        .join(
            ba_dict,
            by = 'billing_account_id',
            type = 'left'
        ) \
        .join(
            puid_dict,
            by = 'puid',
            type = 'left'
        ) \
        .project(
            ne.all(),
            lead_id = ne.custom(lambda email_lead_id, ba_lead_id, puid_lead_id: get_lead_id(email_lead_id, ba_lead_id, puid_lead_id), 'email_lead_id', 'ba_lead_id', 'puid_lead_id')
        ) \
        .filter(
            nf.custom(lambda x: x not in ['', None], 'lead_id')
        ) \
        .unique(
            'lead_id', 'event_time', 'event'
        )
        job.concat(
            leads,
            event
        ) \
        .put(paths_dict['crm_leads_cube'] + '_test')
        job.run()

        job = cluster.job()
        dat = job.table(paths_dict['crm_leads_cube'] + '_test') \
        .groupby(
            'lead_id'
        ) \
        .sort(
            'event_time'
        ) \
        .reduce(
            reduce_func
        ) \
        .project(
            **apply_types_in_project(schema)
        ) \
        .sort(
            'event_time'
        ) \
        .put(paths_dict['crm_leads_cube'], schema = schema)
        job.run()

    except Exception as err:
        text = '''
        Error in smb_calls_report.py:\n\n
        {0}\n
        ============================
        '''.format(str(err))
        requests.post('https://api.telegram.org/bot{0}/sendMessage?chat_id={1}&text={2}'.format(
            telebot_creds['value']['token'],
            telebot_creds['value']['chat_id'],
            '\n'.join(text)
            )
        )


if __name__ == '__main__':
    main()
