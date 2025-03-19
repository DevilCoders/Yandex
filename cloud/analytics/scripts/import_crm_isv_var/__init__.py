import logging

import click

import yt.wrapper as yt

from cloud.analytics.scripts.common.leads_dyn_table import Lead


ATTRIBUTE_FIELDS = ['isv_status', 'var_status']

ba_isv_var_to_dyn_mapping = {
    'isv_status': 'isv_status',
    'var_status': 'var_status',
}


DYN_REQUEST = """
{fields}
    FROM [{dyn_tbl}]
    where dwh_id = {value}
    LIMIT 1
"""


@click.command()
@click.argument('crm_table_path')
@click.argument('dyn_table_path')
@click.option('--proxy', help='YT Proxy', default='hahn')
@click.option('--loglevel', help='Python logging level', default="info")
def main(crm_table_path, dyn_table_path, proxy, loglevel):
    select_fields = Lead.SELECT_FIELDS

    numeric_level = getattr(logging, loglevel.upper(), None)

    if not isinstance(numeric_level, int):
        raise ValueError('Invalid log level: %s' % loglevel)
    logging.basicConfig(level=numeric_level)

    yt.config['proxy']['url'] = proxy
    yt.config['backend'] = 'http'

    write_client = yt.YtClient(config=yt.config.config)
    write_client.config['backend'] = 'rpc'

    read_client = yt.YtClient(config=yt.config.config)
    read_client.config['backend'] = 'rpc'

    for src_row in yt.read_table(crm_table_path):
        value = "'{}'".format(src_row["dwh_id"])
        params = {
            'fields': ', '.join(select_fields),
            'dyn_tbl': dyn_table_path,
            'value': value,
        }
        check_row = list(yt.select_rows(DYN_REQUEST.format(**params), client=read_client))
        item_data = Lead(dyn_table_path, write_client, **check_row[0])
        # update values
        for col_name in ATTRIBUTE_FIELDS:
            if col_name in src_row.keys() and src_row[col_name] is not None:
                setattr(item_data, col_name, src_row[col_name])
        item_data.save()
        logging.info("updating lead with dwh_id={}".format(item_data.dwh_id))
