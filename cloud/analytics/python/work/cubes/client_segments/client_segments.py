#!/usr/bin/env python2
# -*- coding: utf-8 -*-
import pandas as pd, datetime, ast, os, requests, sys
from vault_client import instances
from nile.api.v1 import (
    clusters
)
reload(sys)
sys.setdefaultencoding('utf8')

def main():
    client = instances.Production()
    yt_creds = client.get_version('ver-01d2yggjwgvhjt8814djbc2bp6')
    wiki_creds = client.get_version('ver-01d2z1psntbwweh87na5e6sthy')

    headers = {
        'Authorization': "OAuth %s" % wiki_creds['value']['token']
    }

    WIKI_URI = 'cloud-bizdev/Enterprise-ISV-clients/Spisok-Ent/ISV/'

    out_table = '//home/cloud_analytics/import/wiki/clients_segments'

    url = 'https://wiki-api.yandex-team.ru/_api/frontend/%s/.grid' % WIKI_URI

    r = requests.get(url, headers=headers)
    j = r.json()

    columns = [f['title'] for f in j['data']['structure']['fields']]
    rows = [[c['raw'] for c in row] for row in j['data']['rows']]

    df = pd.DataFrame(rows, columns = ['crm_client_name', 'billing_account_id', 'sales', 'segment'])

    df.crm_client_name = df.crm_client_name.apply(lambda x: x.replace('\"','').replace('«', '').replace('»', ''))

    cluster = clusters.yt.Hahn(
        token = yt_creds['value']['token'],
        pool = yt_creds['value']['pool']
    )

    cluster.write(
        out_table,
        df
    )

if __name__ == '__main__':
    main()
