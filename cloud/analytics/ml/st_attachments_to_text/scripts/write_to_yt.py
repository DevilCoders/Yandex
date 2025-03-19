import zipfile
import json
import os
import re
import sys
from datetime import datetime
import nirvana_dl
import pandas as pd
import logging.config
from clan_tools.logging.logger import default_log_config
from clan_tools.data_adapters.YTAdapter import YTAdapter
from clan_tools.data_adapters.YQLAdapter import YQLAdapter
from clan_tools.data_adapters.TrackerAdapter import TrackerAdapter
import pymorphy2
from clan_tools.secrets.Vault import Vault

logging.config.dictConfig(default_log_config)
logger = logging.getLogger(__name__)

input_path = os.environ['INPUT_DATA_PATH']
# input_path = './'
# Vault().get_secrets()
Vault(token=nirvana_dl.get_options()['user_requested_secret']).get_secrets()
tracker_adapter = TrackerAdapter()
yt_adapter = YTAdapter(token=os.environ['YT_TOKEN'])
yql_adapter = YQLAdapter()
tracker_client = tracker_adapter.st_client

ma = pymorphy2.MorphAnalyzer()

month_dict = {
    "январь":1,
    "февраль":2,
    "март":3,
    "апрель":4,
    "май":5,
    "июнь":6,
    "июль":7,
    "август":8,
    "сентябрь":9,
    "октябрь":10,
    "ноябрь":11,
    "декабрь":12,
}

def clean_text(text):
    text = text.lower()
    text = re.sub('\-\s\r\n\s{1,}|\-\s\r\n|\r\n', ' ', text)
    text = re.sub(
        '[.,:;_%©?*,!@#$%^&(){{}}]|[+=]|[«»]|[<>]|[\']|[[]|[]]|[/]|"|\s{2,}|-', ' ', text)
    text = " ".join(ma.parse(word)[0].normal_form for word in text.split())
    return text

def find_keys(text:str) -> bool:
    text = clean_text(text)
    return (('платёжный поручение' in text)|('invoice' in text)|('чек по операция' in text))

def date_5_days_recent(text:str) -> str:
    match = re.search(r'(\d{2}\.\d{2}\.\d{4})',text)
    if match == None:
        text = clean_text(text)
        match = re.compile(r"\d{1,2} (?:июнь|июль|август|сентябрь|октябрь|ноябрь|декабрь|январь|февраль|март|апрель|май) \d{4}").search(text)
        if match == None:
            return False
        day, month, year = match.group().split()
        datetime_object = datetime(int(year), month_dict[month] , int(day))
        return (datetime.now() - datetime_object).days < 5
    datetime_object = datetime.strptime(match.group(), '%d.%m.%Y')
    return (datetime.now() - datetime_object).days < 5


def billing(data):
    to_send = data[data['file_type'] == 'pdf']
    to_send['num_pdfs'] = to_send.groupby('st_key')['attachment'].transform('count')
    to_send = to_send[to_send['num_pdfs'] <= 2]
    to_send['contains_invoice'] = to_send['text'].apply(find_keys)
    to_send['invoice_date'] = to_send['text'].apply(date_5_days_recent)
    logger.debug(to_send)
    to_send = to_send[to_send.groupby('st_key')['contains_invoice'].transform('sum') == to_send['num_pdfs']]
    if len(to_send) == 0:
        return
    to_send_keys = to_send[to_send['invoice_date']]['st_key'].unique()
    df = pd.DataFrame(to_send_keys, index=to_send_keys, columns=['key'])
    logger.debug(to_send_keys)
    df['billing_account_id'] = None
    for key in to_send_keys:
        for comment in tracker_client.issues[key].comments.get_all():
            text = comment.text
            try:
                s = 'https://backoffice.cloud.yandex.ru/billing/accounts/'
                text = text[text.index(s)+len(s):]
                df.loc[key, 'billing_account_id'] = text[:text.index(' ')]
            except:
                pass
    ba_info = yql_adapter.execute_query('''
    Use hahn;
    SELECT `billing_account_id`, `state`
    FROM `//home/cloud-dwh/data/prod/ods/billing/billing_accounts`
    ''', to_pandas=True)
    df = pd.merge(df, ba_info, how='left', on='billing_account_id')
    logger.debug(df)
    st_keys = df[df['state'] != 'suspended']['key']
    logger.debug(f"TICKETS: {st_keys}")
    for key in st_keys: 
        issue = tracker_client.issues[key]
        support_components = list(map(lambda x: x.as_dict()['display'], issue.components))
        if 'billing.wire_transfers' not in support_components:
            text = issue.summary + ' ' + issue.description
            text = clean_text(text)
            if all([x not in text for x in ['не проходит оплата', 'не могу оплатить', 'не получается оплатить']]):
                logger.debug(key)
                issue.update(components={'set': ['billing.wire_transfers']})

def parse_string(x):
    s = x
    file_type = s[:s.index('_')]
    s = s[s.index('_')+1:]
    ticket = s[:s.index('_')]
    s = s[s.index('_')+1:]
    if file_type =='pdf':
        attach = s[:s.index('-')]
        page = s[s.index('-')+1:s.index('.')]
    else:
        attach = s[:s.index('.')]
        page = '0'
    return file_type, ticket, attach, page


def main():
    # logger.debug(os.listdir(input_path))
    df = pd.DataFrame(columns=['st_key', 'attachment', 'page', 'text', 'file_type'])
    for x in os.listdir(input_path): 
        try: 
            file_type, ticket, attach, page = parse_string(x)
            with open(input_path + '/' + x, 'r') as f:
                txtdata = f.read()
            jsondata = json.loads(txtdata)
            text = ''
            for s in jsondata['data']['fulltext']:
                text+=s['Text']
            new_row = {'st_key':ticket, 'attachment':attach, 'page':page, 'text':text, 'file_type':file_type}
            df = df.append(new_row, ignore_index=True)
        except:
            pass
    df = df.sort_values('page').groupby(['st_key', 'attachment', 'file_type'])['text'].sum().reset_index()
    logger.debug('before billing:')
    logger.debug(df)
    billing(df)
    yt_schema = [{'name': 'st_key', 'type': 'string'}, {'name': 'attachment', 'type': 'string'}, {'name': 'text', 'type': 'string'}]
    yt_adapter.save_result("//home/cloud_analytics/ml/st_attachment_to_text/CLOUDSUPPORT", yt_schema, df[['st_key', 'attachment', 'text']], append=True)

if __name__ == '__main__':
    main()
