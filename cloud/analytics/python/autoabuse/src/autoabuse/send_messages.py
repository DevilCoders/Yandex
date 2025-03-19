
from datetime import datetime
import logging
import pandas as pd
from numpy.lib.function_base import append
from clan_tools.data_adapters.YTAdapter import YTAdapter
import time
logger = logging.getLogger(__name__)


header = '''#|
|| IP  | ВМ | Аккаунт | Облако |Траффик с  | Траффик по | Кол-во сканов IP | Реакция ||
'''

clouds_url = 'https://backoffice.cloud.yandex.ru/clouds'
ba_url =  'https://backoffice.cloud.yandex.ru/billing/accounts'
footer = '|#'
row_pattern = (
    '|| {sourceip} '
    ' | (({clouds_url}/{cloud_id}/compute/{instance_id} {instance_id})) '
    ' | (({ba_url}/{billing_account_id} {billing_account_id})) {ba_info} ' 
    ' | (({clouds_url}/{cloud_id} {cloud_id})) ' 
    ' | {min_setup_time} '
    ' | {max_setup_time} '
    ' | {n_unique_ssh_rdp_dests} '
    ' | {reaction} ||'
)


def create_message_body(row):
    format_dict = row.to_dict()
    format_dict['clouds_url'] = clouds_url
    format_dict['ba_url'] = ba_url
    format_dict['ba_info'] = '\n {segment} {ba_usage_status}'.format(**format_dict)
    format_dict['reaction'] = 'Блок' if format_dict['block'] else ''

    body = row_pattern.format(**format_dict)
    return body


def prepare_messages(keys_traffic):
    messages = keys_traffic.apply(create_message_body, axis=1)
    keys_traffic['message_body'] = messages
    keys_messages = (
        keys_traffic.groupby('keys')['message_body'].apply(
        lambda x: header + '\n'.join(x) + footer)
        .sort_index()
    )
    close_tickets = (
        keys_traffic.groupby('keys')['block']
        .apply(lambda x: sum(~x.values))
        .sort_index()==0

    )
    statuses = keys_traffic.groupby('keys')['ba_usage_status'].unique().sort_index()
    segments = keys_traffic.groupby('keys')['segment'].unique().sort_index()
    return    pd.concat([keys_messages, close_tickets, statuses, segments], axis=1)





def send_message(row):
    row.issue.comments.create(text=row.message_body)
    trials = 0 
    while trials < 3:
        try:
            if row['block']:
                row.issue.transitions['resolve'].execute()
            row.issue.update(tags={
                'add': row['ba_usage_status'].tolist() +row['segment'].tolist()
            })

            return
        except Exception as e:
            logger.error(e, exc_info=True)
        finally:
            trials += 1

def write_label_history(keys_messages, history_path):
    labeled_tickets = keys_messages[['key']].copy()
    labeled_tickets['label_time'] = datetime.now().timestamp()
    YTAdapter().save_result(history_path, 
                       schema={'key':'string', 'label_time':'double'},
                       df=labeled_tickets)

def suspend_trial(suspend_table_path, accounts):
    accounts_to_suspend = (
        accounts[accounts.block == True][['billing_account_id']]
        .drop_duplicates()
        .copy()
    )
    
    accounts_to_suspend['generated_at'] = int(time.time())
    accounts_to_suspend['suspend_after'] = 3600

    YTAdapter().save_result(suspend_table_path, 
        schema=[
            {"name": 'billing_account_id', 'type': 'string', 'required': False},
            {"name": 'generated_at', 'type': 'uint64', 'required': True},
            {"name": 'suspend_after', 'type': 'int32', 'required': True},
        ],
        df=accounts_to_suspend, append=True
    )
