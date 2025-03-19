#!/usr/bin/env python3
import os

import requests
import yt.wrapper as yt

DST_CLUSTER = 'hume'

WIKI_API_URL = "https://wiki-api.yandex-team.ru/_api/frontend"
WIKI_OAUTH_TOKEN = os.environ.get('WIKI_OAUTH_TOKEN')
TABLE_LIST_TAG = 'cloud/bizdev/automation/datalake/staging/marketo2dwh'
COMMON_FIELDS_TAG = 'cloud/bizdev/automation/leadonance/obshhie-polja-aktivnostejj-marketo'

MARKETO_TO_YT_TYPES = {
    'integer': 'int64',
    'string': 'string',
    'float': 'double',
    'datetime iso8601': 'string',
    'email': 'string',
    'stirng': 'string',
    'boolean': 'boolean',
    'bolean': 'boolean',
    'text': 'string',
    'list of integers': 'any',
    'list of strings': 'any',
    'date iso8601': 'string',
}

yt.config["proxy"]["url"] = DST_CLUSTER


def wiki_api(supertag, method, params={}):
    headers = {
        'Authorization': 'OAuth '+WIKI_OAUTH_TOKEN,
    }
    r = requests.get("{wiki_api_url}/{supertag}/.{method}".format(wiki_api_url=WIKI_API_URL, supertag=supertag, method=method), headers=headers, params=params)
    return r.json()['data']


def get_fields(tag):
    fields = {}
    data = wiki_api(tag, 'grid')
    for row in data['rows']:
        required = False
        if row[2]['raw'] == 'да':
            required = True
        field = {'name': row[0]['raw'], 'type': MARKETO_TO_YT_TYPES[row[1]['raw']], 'required': required}
        fields[row[0]['raw']] = field
    return fields


common_schema = get_fields(COMMON_FIELDS_TAG)
for entry in wiki_api(TABLE_LIST_TAG, 'tree')['subpages']:
    if 'polja-aktivnosti' in entry['page']['url']:
        schema = get_fields(entry['page']['url'])
        table_name = os.path.basename(entry['page']['url']).replace('polja-aktivnosti-', '').replace('-', '_')
        print(table_name)
        dst_table = '//projects/marketo-testing/import/{}'.format(table_name)
        if not yt.exists(dst_table):
            yt.create('table', dst_table, attributes={'schema': list({**common_schema, **schema}.values())})
