import logging
import time

import click
import hashlib

import yt.wrapper as yt
from cloud.analytics.scripts.common.leads_dyn_table import \
    Lead, index_table_name

ANONYMIZED_EMAIL = "robot-clanalytics-yt@yandex-team.ru"

ANONYMIZED_FIELDS = [
    'email',
    'first_name',
    'last_name',
    'crm_account_name',
    'crm_email',
    'crm_description',
    'crm_phone_home',
    'crm_phone_mobile',
    'crm_phone_other',
    'crm_phone_work',
    'crm_primary_address_street',
    'crm_primary_address_postalcode',
    'crm_primary_address_state',
    'crm_title',
    'forms_company',
    'forms_email',
    'forms_phone',
    'forms_position',
    'mkto_city',
    'mkto_email',
    'mkto_company',
    'mkto_address',
    'mkto_client_name',
    'website'
]

LOOKUP_FIELDS = {
    'passport_uid': 'passport_uid',
    'user_settings_email': 'email',
}

DYN_REQUEST = """
{fields} FROM [{dyn_ind_tbl}] AS i
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
@click.argument('removed_lead_table_path')
@click.argument('dyn_table_path')
@click.option('--proxy', help='YT Proxy', default='hahn')
@click.option('--loglevel', help='Python logging level', default="info")
def main(removed_lead_table_path, dyn_table_path, proxy, loglevel):
    NON_REWRITABLE_FIELDS = ['passport_uid']
    select_fields = Lead.SELECT_FIELDS + ANONYMIZED_FIELDS + ['is_anonymized']

    numeric_level = getattr(logging, loglevel.upper(), None)
    if not isinstance(numeric_level, int):
        raise ValueError('Invalid log level: %s' % loglevel)
    logging.basicConfig(level=numeric_level)

    yt.config['proxy']['url'] = proxy
    yt.config['backend'] = 'http'

    mount_dyn_table(dyn_table_path)
    for field in LOOKUP_FIELDS.values():
        dyn_ind_tbl = index_table_name(dyn_table_path, field, Lead.PK)
        mount_dyn_table(dyn_ind_tbl)

    read_client = yt.YtClient(config=yt.config.config)
    read_client.config['backend'] = 'rpc'

    write_client = yt.YtClient(config=read_client.config)

    for src_row in yt.read_table(removed_lead_table_path):
        rows = {}

        for src_field, dst_field in LOOKUP_FIELDS.items():
            if src_field not in src_row or not src_row[src_field]:
                continue

            value = src_row[src_field]
            if dst_field not in Lead.INT_FIELDS:
                value = "'{}'".format(value)

            params = {
                'fields': ', '.join(select_fields),
                'dyn_ind_tbl': index_table_name(dyn_table_path, dst_field, Lead.PK),
                'dyn_tbl': dyn_table_path,
                'dst_field': dst_field,
                'value': value,
            }

            check_rows = list(yt.select_rows(DYN_REQUEST.format(**params), client=read_client))
            for row in check_rows:
                if 'is_anonymized' in row and row['is_anonymized']:
                    logging.debug('Founded already anonymized lead')
                    continue

                logging.debug('Founded row: %s' % row)
                rows[row['dwh_id']] = row

        for row in rows.values():
            item_data = Lead(dyn_table_path, write_client, **row)

            # anonymize data
            for field in ANONYMIZED_FIELDS:
                if not row[field]:
                    continue
                row[field] = hashlib.md5(row[field].encode('utf-8')).hexdigest()

            row['is_anonymized'] = True
            row['user_settings_email'] = 'robot-clanalytics-yt@yandex-team.ru'

            logging.info('Insert specific data: {}'.format(row))
            item_data.from_dict(row, None, NON_REWRITABLE_FIELDS)
            item_data.save()
