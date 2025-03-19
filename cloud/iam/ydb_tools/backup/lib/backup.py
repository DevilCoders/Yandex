# coding=utf-8

import logging
import os
import datetime

import cloud.iam.ydb_tools.backup.lib.ydb_reader as ydb_reader
import yt.wrapper as yt

logger = logging.getLogger(__name__)


def create_yt_table(yt_client, table_schema, table_name):
    assert not yt_client.exists(table_name)
    yt_client.create(
        "table",
        table_name,
        recursive=True,
        attributes=dict(schema=table_schema, optimize_for="scan"),
    )


def convert_to_yt_format(table_schema):
    TYPES_MAPPING = {
        'bool': 'boolean'
    }
    for column in table_schema:
        if column.pop("primary_key", False):
            column["sort_order"] = "ascending"
        column['type'] = TYPES_MAPPING.get(column['type'], column['type'])
    return table_schema


def upload_to_yt(yt_backup_dir, fs_dir, yt_tablename, fs_tablename, filter_columns=None):
    logger.info('Start write table "%s"', fs_tablename)
    yt_client = yt.YtClient(proxy=yt.config["proxy"]["url"], token=yt.config["token"])
    yt_table = os.path.join(yt_backup_dir, yt_tablename)
    schema, rows_generator = ydb_reader.read_table(os.path.join(fs_dir, fs_tablename), filter_columns)
    schema = [item for item in schema if filter_columns is None or item['name'] in filter_columns]
    create_yt_table(yt_client, convert_to_yt_format(schema), yt_table)
    yt_client.write_table(yt_table, rows_generator())
    logger.info('Finish write table "%s"', fs_tablename)


def backup(yt_directory, fs_directory, tables, ttl_days):
    # timestamp = str(int(time.time()))
    YT_DATETIME_FORMAT_STRING = "%Y-%m-%dT%H:%M:%S.%fZ"
    timestamp = datetime.datetime.utcnow().strftime(YT_DATETIME_FORMAT_STRING)

    backup_dir = os.path.join(yt_directory, timestamp)
    yt.mkdir(backup_dir)
    yt.set_attribute(backup_dir, 'backup_timestamp', timestamp)
    ts_exp = datetime.datetime.utcnow() + datetime.timedelta(days=ttl_days)
    yt.set_attribute(backup_dir, "expiration_time", ts_exp.isoformat())

    for table in tables:
        fs_tablename = table[0]
        yt_tablename = table[1]
        if table[2] == "":
            columns = None
        else:
            columns = table[2].split(",")
        upload_to_yt(backup_dir, fs_directory, yt_tablename, fs_tablename, columns)
    yt.link(backup_dir, os.path.join(yt_directory, "snapshot"), recursive=True, force=True)
    # futures = list()
    # with concurrent.futures.ThreadPoolExecutor(max_workers=args.parallel) as executor:
    #    for table in os.listdir(temp_dir):
    #        futures.append(executor.submit(upload_to_yt, table))
    # for future in futures:
    #    future.result()
