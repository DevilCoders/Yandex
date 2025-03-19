import logging
import time

import click

import yt.wrapper as yt
from cloud.analytics.scripts.common.leads_dyn_table import \
    crm_to_dyn_mapping, Lead, index_table_name

LOOKUP_FIELDS = {
    'id': 'crm_id',
    'passport_uid': 'passport_uid',
    'primary_email': 'email',
}

DYN_REQUEST = """
crm_date_modified, {fields}
    FROM [{dyn_ind_tbl}] AS i
    JOIN [{dyn_tbl}]
    ON i.dwh_id = dwh_id
    WHERE i.{dst_field} = {value} AND {dst_field} = {value}
    ORDER BY crm_date_modified
    DESC LIMIT 1
"""


def mount_dyn_table(table):
    yt.mount_table(table)
    while yt.get('{0}/@tablets/0/state'.format(table)) != 'mounted':
        time.sleep(0.1)


@click.command()
@click.argument('crm_tables_path')
@click.argument('dyn_table_path')
@click.option('--proxy', help='YT Proxy', default='hahn')
@click.option('--loglevel', help='Python logging level', default="info")
def main(crm_tables_path, dyn_table_path, proxy, loglevel):
    NON_REWRITABLE_FIELDS = [
        'passport_uid',
        'mkto_id',
        'account_id',
        'ba_id',
        'first_name',
        'last_name',
        'preferred_language',
        'website',
    ]
    select_fields = Lead.SELECT_FIELDS + NON_REWRITABLE_FIELDS + ['is_anonymized']

    numeric_level = getattr(logging, loglevel.upper(), None)
    if not isinstance(numeric_level, int):
        raise ValueError('Invalid log level: %s' % loglevel)
    logging.basicConfig(level=numeric_level)

    yt.config['proxy']['url'] = proxy
    yt.config['backend'] = 'http'

    tables_history = crm_tables_path + '/history/leads'

    if not yt.exists(tables_history):
        yt.create(
            'table',
            tables_history,
            recursive=True
        )
    mount_dyn_table(dyn_table_path)
    for field in LOOKUP_FIELDS.values():
        dyn_ind_tbl = index_table_name(dyn_table_path, field, Lead.PK)
        mount_dyn_table(dyn_ind_tbl)

    new_tables = list()

    for cypress_node in yt.list(crm_tables_path):
        if cypress_node == 'history':
            continue
        new_tables.append(
            '/'.join([crm_tables_path, cypress_node]))

    if not new_tables:
        return

    read_client = yt.YtClient(config=yt.config.config)
    read_client.config['backend'] = 'rpc'

    write_client = yt.YtClient(config=read_client.config)

    for table in new_tables:
        for src_row in yt.read_table(table):
            item_data = None
            already_filled = False

            if 'primary_email' in src_row:
                src_row['primary_email'] = Lead.clean_email(src_row['primary_email'])

            for src_field, dst_field in LOOKUP_FIELDS.items():
                if src_field not in src_row or not src_row[src_field]:
                    continue

                value = src_row[src_field]
                if dst_field not in Lead.INT_FIELDS:
                    value = "'{}'".format(value.replace("'", "\\'"))

                params = {
                    'fields': ', '.join(select_fields),
                    'dyn_ind_tbl': index_table_name(dyn_table_path, dst_field, Lead.PK),
                    'dyn_tbl': dyn_table_path,
                    'dst_field': dst_field,
                    'value': value,
                }

                check_row = list(yt.select_rows(DYN_REQUEST.format(**params), client=read_client))

                if not check_row or ('is_anonymized' in check_row and check_row['is_anonymized']):
                    continue

                logging.info('Matched lead by {}: {}'.format(dst_field, check_row))
                item_data = Lead(dyn_table_path, write_client, **check_row[0])
                break

            if already_filled:
                continue

            if item_data is None:
                logging.info('Cannot match any, adding new lead')
                item_data = Lead(dyn_table_path, write_client)
                item_data.email = src_row['primary_email']

            item_data.from_dict(src_row, crm_to_dyn_mapping, NON_REWRITABLE_FIELDS)
            logging.debug('Insert specific data: %s' % item_data)
            item_data.save()
        logging.info('Archiving %s' % table)
        yt.write_table(
            yt.TablePath(tables_history, append=True),
            list(yt.read_table(table))
        )
        yt.remove(table)
