import logging
import time

import click

import yt.wrapper as yt
from cloud.analytics.scripts.common.leads_dyn_table import Lead

DYN_REQUEST = "{fields} FROM [{dyn_tbl}] WHERE dwh_id = \"{value}\""


def mount_dyn_table(table):
    yt.mount_table(table)
    while yt.get('{0}/@tablets/0/state'.format(table)) != 'mounted':
        time.sleep(0.1)


@click.command()
@click.argument('lead_id_table_path')
@click.argument('dyn_table_path')
@click.option('--proxy', help='YT Proxy', default='hahn')
@click.option('--loglevel', help='Python logging level', default="info")
def main(lead_id_table_path, dyn_table_path, proxy, loglevel):
    select_fields = Lead.SELECT_FIELDS

    numeric_level = getattr(logging, loglevel.upper(), None)
    if not isinstance(numeric_level, int):
        raise ValueError('Invalid log level: %s' % loglevel)
    logging.basicConfig(level=numeric_level)

    yt.config['proxy']['url'] = proxy
    yt.config['backend'] = 'http'

    mount_dyn_table(dyn_table_path)

    read_client = yt.YtClient(config=yt.config.config)
    read_client.config['backend'] = 'rpc'

    write_client = yt.YtClient(config=read_client.config)

    for src_row in yt.read_table(lead_id_table_path):
        if not src_row['mkto_lead_id'] and not src_row['mkto_id']:
            continue
        item_data = None

        params = {
            'fields': ', '.join(select_fields),
            'dyn_tbl': dyn_table_path,
            'value': src_row['dwh_id'],
        }

        check_row = list(yt.select_rows(DYN_REQUEST.format(**params), client=read_client))

        if not check_row:
            continue

        logging.info('Matched lead by dwh_id: {}'.format(check_row))
        item_data = Lead(dyn_table_path, write_client, **check_row[0])
        item_data.mkto_lead_id = src_row['mkto_lead_id']
        item_data.mkto_id = src_row['mkto_id']
        item_data.save(with_update=False)
