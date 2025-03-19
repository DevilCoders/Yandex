import logging
from datetime import datetime, timedelta

import click

import yt.wrapper as yt
from yql.api.v1.client import YqlClient

from cloud.analytics.scripts.common.leads_dyn_table import Lead, index_table_name
from yt.yson import YsonUint64

LOOKUP_FIELDS = {
    'passport_uid': 'passport_uid',
    'forms_email': 'email',
}

forms_to_dyn_mapping = {
    'forms_created_time': 'forms_created_time',
    'passport_uid': 'passport_uid',
    'forms_company': 'forms_company',
    'forms_phone': 'forms_phone',
    'forms_email': 'email',
    'forms_position': 'forms_position',
    'forms_first_name': 'first_name',
    'forms_last_name': 'last_name',
    'forms_mail_marketing': 'forms_mail_marketing',
}

INT_FIELDS = ['passport_uid']

DYN_REQUEST = """
forms_created_time, {fields}
    FROM [{dyn_ind_tbl}] AS i
    JOIN [{dyn_tbl}]
    ON i.dwh_id = dwh_id
    WHERE i.{dst_field} = {value} AND {dst_field} = {value}
    ORDER BY forms_created_time
    DESC LIMIT 1
"""


def mount_dyn_table(table):
    logging.debug('Mounting table: {}'.format(table))
    yt.mount_table(table, sync=True)


def coalesce(row, column_name, value_if_null):
    if getattr(row, column_name, None) is None:
        setattr(row, column_name, value_if_null)


@click.command()
@click.argument('forms_table_path')
@click.argument('dyn_table_path')
@click.option('--proxy', help='YT Proxy', default='hahn')
@click.option('--loglevel', help='Python logging level', default="info")
def main(forms_table_path, dyn_table_path, proxy, loglevel):
    NON_REWRITABLE_FIELDS = [
        'passport_uid',
        'email',
        'first_name',
        'last_name',
    ]

    select_fields = Lead.SELECT_FIELDS + NON_REWRITABLE_FIELDS + ['forms_position', 'forms_company','forms_email',
                                                                  'forms_phone', 'forms_mail_marketing']
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

    from yql.api.v1.client import YqlClient

    read_client = yt.YtClient(config=yt.config.config)
    read_client.config['backend'] = 'rpc'

    yql_client = YqlClient(db='hahn')
    write_client = yt.YtClient(config=read_client.config)

    query = f'SELECT * FROM `{forms_table_path}` '\
            'where passport_uid is not null or forms_email is not null '\
            'ORDER BY passport_uid, forms_email, forms_created_time '

    request = yql_client.query(query, syntax_version=1)
    request.run()

    table = request.table

    table.fetch_full_data()

    for row in table.rows:
        src_row = dict(zip(table.column_names, row))
        item_data = None

        if src_row['forms_created_time'] is not None:
            src_row['forms_created_time'] = int(src_row['forms_created_time'] / 1000)

        if src_row['forms_email'] is not None:
            src_row['forms_email'] = Lead.clean_email(src_row['forms_email'])

        for src_field, dst_field in LOOKUP_FIELDS.items():
            if src_field not in src_row or src_row[src_field] is None:
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

            print(DYN_REQUEST.format(**params))

            check_row = list(yt.select_rows(DYN_REQUEST.format(**params), client=read_client))

            if check_row:
                print('Matched lead by {}: {}'.format(dst_field, check_row))
                item_data = Lead(dyn_table_path, write_client, **check_row[0])
                break
            else:
                print("No match")

        if item_data is None:
            print('Cannot match any, adding new lead')
            item_data = Lead(dyn_table_path, write_client)
            item_data.from_dict(src_row, forms_to_dyn_mapping)
            coalesce(item_data, "forms_email", src_row["forms_email"])
            print(f"Adding new lead: {item_data._data}")
        else:
            print("Updating lead.")
            print(f"Old value: {item_data._data}")

            # fill in nulls
            for col_name in ['forms_created_time', 'forms_position', 'forms_company', 'forms_email', 'forms_phone',
                             'forms_mail_marketing', 'passport_uid']:
                if src_row[col_name] is not None:
                    coalesce(item_data, col_name, src_row[col_name])

            coalesce(item_data, "email", src_row["forms_email"])

            # update values
            for col_name in ['forms_created_time', 'forms_position', 'forms_company', 'forms_email', 'forms_phone',
                             'forms_mail_marketing']:
                if src_row[col_name] is not None:
                    setattr(item_data, col_name, src_row[col_name])

            print(f'new value: {item_data._data}')

        item_data.save()
