#!/usr/bin/env python
import os
import sys
import logging

logging.basicConfig(level=logging.INFO)

import requests
import yt.wrapper as yt

DST_CLUSTER = 'hume'

WIKI_API_URL = "https://wiki-api.yandex-team.ru/_api/frontend"
WIKI_OAUTH_TOKEN = os.environ.get('WIKI_OAUTH_TOKEN')
TABLE_PAGE = sys.argv[1]

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
        name = row[3]['raw']
        logging.info('Parsed column name: {}'.format(name))
        if name == '--':
            continue
        required = False
        if row[5]['raw'] == 'true':
            required = True
        field = {'name': name, 'type': row[4]['raw'], 'required': required}
        fields[name] = field
    logging.info('Parsed schema: {}'.format(fields))
    return fields


logging.info('Gettting schema from wiki page: {}'.format(TABLE_PAGE))
schema = get_fields(TABLE_PAGE)
table_name = os.path.basename(TABLE_PAGE).replace('-', '_')
logging.info('Importing table: {}'.format(table_name))
dst_table = '//projects/marketo-testing/export/{}'.format(table_name)
if not yt.exists(dst_table):
    yt.create('table', dst_table, attributes={'schema': list(schema.values())})
