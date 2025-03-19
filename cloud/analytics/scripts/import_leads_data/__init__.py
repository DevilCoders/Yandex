import logging
import os

import click
import yt.wrapper as yt
from yt.wrapper.default_config import retries_config
from cloud.analytics.scripts.common.leads_dyn_table import iam_to_dyn_mapping, Lead, index_table_name
from tqdm import tqdm

LOGGER = logging.getLogger()

LOOKUP_FIELDS_MAPPING = {
    'passport_uid': 'passport_uid',
    'user_settings_email': 'email',
}

TRANSPORT_ERROR = 100  # aka Connection refused


def check_update_row(dst_table, row, read_client, write_client, write_statistics):
    updated_row = None
    fields = [iam_to_dyn_mapping[field] for field in row.keys()] + [Lead.PK] + Lead.REQUIRED_FIELDS
    for iam_field, dyn_field in LOOKUP_FIELDS_MAPPING.items():
        iam_field_value = row[iam_field]
        if type(iam_field_value) == str:
            value = f'"{iam_field_value}"'
        else:
            value = str(iam_field_value)
        predicate = f'i.{dyn_field} = {value} AND {dyn_field} = {value}'
        existing_rows = list(yt.select_rows(
            "{} FROM [{}] AS i JOIN [{}] ON i.dwh_id = dwh_id WHERE {}".format(', '.join(fields),
                                                                               index_table_name(dst_table,
                                                                                                dyn_field, Lead.PK),
                                                                               dst_table, predicate),
            timestamp=yt.ASYNC_LAST_COMMITED_TIMESTAMP, client=read_client))
        if len(existing_rows) > 1:
            LOGGER.warning(existing_rows)
        assert len(existing_rows) <= 1
        if len(existing_rows) > 0:
            updated_row = Lead(dst_table, write_client, **existing_rows[0])
            updated_row.from_dict(row, iam_to_dyn_mapping)
            updated_row.email = row['user_settings_email']
            updated = updated_row.save()
            if updated:
                write_statistics({"updated_rows": 1})
            break
    if not updated_row:
        new_row = Lead(dst_table, write_client)
        new_row.from_dict(row, iam_to_dyn_mapping)
        new_row.email = row['user_settings_email']
        inserted = new_row.save()
        if inserted:
            write_statistics({"new_rows": 1})
    write_statistics({"seen_rows": 1})


def import_data_wrapper(dst_table, proxy, local):
    def import_data(row):
        def yt_write_statistics(data):
            return yt.write_statistics(data)

        def local_write_statistics(data):
            pass

        write_statistics = yt_write_statistics

        if local:
            write_statistics = local_write_statistics

        read_client = yt.YtClient(proxy, config={"backend": "rpc", "dynamic_table_retries": retries_config(count=12, enable=True, backoff={
            "policy": "exponential",
            "exponential_policy": {
                "start_timeout": 5000,
                "base": 2,
                "max_timeout": 120000,
                "decay_factor_bound": 0.3
            }
            },
            additional_retriable_error_codes=[TRANSPORT_ERROR]
        )})
        if 'YT_SECURE_VAULT_yt_token' in os.environ:
            read_client.config['token'] = os.environ['YT_SECURE_VAULT_yt_token']
        write_client = yt.YtClient(config=read_client.config)
        check_update_row(dst_table, row, read_client, write_client, write_statistics)

    return import_data


@click.command()
@click.argument('src_table_prefix')
@click.argument('dst_table')
@click.option('--proxy', help='YT Proxy', default='hahn')
@click.option('--local', help='Launch job locally, instead of map job', is_flag=True)
def main(src_table_prefix, dst_table, proxy, local):
    yt.config['proxy']['url'] = proxy
    yt.config['allow_http_requests_to_yt_from_job'] = True

    mapper = import_data_wrapper(dst_table, proxy, local)

    non_empty_tables = list(filter(lambda t: t.attributes['row_count'] > 0, yt.list(src_table_prefix,
                                                                                    attributes=['row_count'],
                                                                                    sort=True, absolute=True)))
    src_table = non_empty_tables[-1]

    if local:
        for row in tqdm(yt.read_table(src_table),
                        total=yt.get(src_table + '/@row_count')):
            mapper(row)
    else:
        token = yt.http_helpers.get_token(client=yt.config)
        yt.run_map(mapper, src_table, '//tmp/waiting-for-YT-10799',
                   spec={'secure_vault': {'yt_token': token}, 'mapper': {'cpu_limit': 0.5}, 'max_speculative_job_count_per_task': 0})
