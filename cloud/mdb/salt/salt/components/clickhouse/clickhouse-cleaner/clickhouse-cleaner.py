#!/usr/bin/env python3
"""
Clean up old data based on retention policies.
"""
import json
import logging
import subprocess
from pathlib import Path

import yaml

LOG_FILE = '/var/log/yandex/clickhouse-cleaner.log'
CONFIG_FILE = '/etc/yandex/clickhouse-cleaner/clickhouse-cleaner.conf'


def main():
    """
    Program entry point.
    """
    logging.basicConfig(filename=LOG_FILE, level='DEBUG', format='%(asctime)s [%(levelname)s]: %(message)s')

    logging.info('Cleanup started')

    try:
        config = get_config()

        for retention_policy in config.get('retention_policies', []):
            enforce_policy(retention_policy)

        logging.info('Cleanup completed')

    except Exception:
        logging.exception('Cleanup failed with:')
        raise


def get_config():
    """
    Read and parse config file.
    """
    with open(CONFIG_FILE, 'r') as f:
        return yaml.load(f)


def enforce_policy(retention_policy):
    """
    Enforce retention policy.
    """
    logging.info('Processing retention policy: %s', retention_policy)

    database = retention_policy['database']
    table_name = retention_policy.get('table')
    max_partition_count = retention_policy['max_partition_count']
    max_table_size = retention_policy.get('max_table_size')
    table_rotation_suffix = retention_policy.get('table_rotation_suffix')

    if table_rotation_suffix and not table_name:
        raise RuntimeError('The parameter "table_rotation_suffix" cannot be used without "table"')

    for table in get_tables(database, table_name):
        enforce_policy_for_table(database, table, max_partition_count, max_table_size, table_rotation_suffix)


def enforce_policy_for_table(database, table, max_partition_count, max_table_size, table_rotation_suffix):
    """
    Enforce retention policy for the specified table.
    """
    tables = [table]
    if table_rotation_suffix:
        tables += get_rotated_tables(database, table['name'], table_rotation_suffix)

    counter = max_partition_count
    size_left = max_table_size
    for i_table in tables:
        if (max_partition_count and counter == 0) or (max_table_size and size_left == 0):
            drop_table(database, i_table)
            continue

        partitions = get_partitions(database, i_table)

        if max_partition_count:
            if len(partitions) > counter:
                drop_partitions(database, i_table, partitions[counter:])
                counter = 0
            else:
                counter -= len(partitions)

        if max_table_size:
            size = get_partitions_size(database, i_table)
            if size > size_left:
                pos = 0
                while size_left >= 0:
                    size_left = size_left - int(partitions[pos]['size'])
                    pos += 1
                drop_partitions(database, i_table, partitions[pos:])
                size_left = 0
            else:
                size_left -= size


def get_tables(database, table_name=None):
    """
    Get list of tables.
    """
    query = '''
        SELECT name, partition_key
        FROM system.tables
        WHERE database = '{database}'
          AND match(engine, 'MergeTree')
        '''.format(database=database)
    if table_name:
        query += '  AND name = \'{table}\''.format(table=table_name)

    return execute_query(query, 'JSON')['data']


def get_rotated_tables(database, table_name, table_rotation_suffix):
    """
    Get list of rotated tables.
    """
    query = '''
        SELECT name, partition_key
        FROM system.tables
        WHERE database = '{database}'
          AND match(engine, 'MergeTree')
          AND match(name, '^{table_basename}{table_suffix}$')
        ORDER BY name DESC
        '''.format(
        database=database,
        table_basename=table_name,
        table_suffix=table_rotation_suffix)

    return execute_query(query, 'JSON')['data']

def get_partitions_size(database, table):
    """
    Return sum of sizes of partitions within the specified table.
    """
    query = '''
        SELECT SUM(bytes_on_disk) "size"
        FROM system.parts
        WHERE database = '{database}'
          AND table = '{table}'
          AND active
        '''.format(
        database=database,
        table=table['name'])
    return int(execute_query(query, 'JSON')['data'][0]['size'])

def get_partitions(database, table):
    """
    Return sizes of partitions within the specified table.
    """
    query = '''
        SELECT
          partition "name",
          SUM(bytes_on_disk) "size"
        FROM system.parts
        WHERE database = '{database}'
          AND table = '{table}'
          AND active
        GROUP BY partition
        HAVING MIN(min_date) <= today()
          AND MIN(min_time) <= now()
        ORDER BY partition
        DESC
        '''.format(
        database=database,
        table=table['name'])
    return execute_query(query, 'JSON')['data']

def drop_table(database, table):
    """
    Drop the specified table.
    """
    logging.info('Dropping table: `%s`.``%s', database, table['name'])

    enable_force_drop_table()

    query = 'DROP TABLE `{database}`.`{table}`'.format(database=database, table=table['name'])
    execute_query(query)


def drop_partitions(database, table, partitions):
    """
    Drop the specified table partitions.
    """
    partition_names = [p['name'] for p in partitions]

    logging.info('Dropping table partitions, table: `%s`.`%s`, partitions: %s', database, table['name'],
                 partition_names)

    enable_force_drop_table()

    query = 'ALTER TABLE `{database}`.`{table}` DROP PARTITION '.format(database=database, table=table['name'])
    if get_partition_key_type(database, table) in ('String', 'DateTime', 'Date'):
        query += '\'{partition}\''
    else:
        query += '{partition}'

    for partition_name in partition_names:
        execute_query(query.format(partition=partition_name))


def get_partition_key_type(database, table):
    """
    Get partition key type.
    """
    query = 'SELECT {partition_key} FROM `{database}`.`{table}` LIMIT 0'.format(
        database=database,
        table=table['name'],
        partition_key=table['partition_key'])
    return execute_query(query, 'JSON')['meta'][0]['type']


def execute_query(query, format=None):
    """
    Execute ClickHouse query.
    """
    cmd = 'clickhouse-client --disable_suggestion'
    if format:
        cmd += ' --format {0}'.format(format)

    proc = subprocess.Popen(cmd, shell=True, stdin=subprocess.PIPE,
                            stdout=subprocess.PIPE, stderr=subprocess.PIPE)

    stdout, stderr = proc.communicate(input=query.encode())

    response = stdout.decode()

    logging.debug('%s:\nquery: %s\nresult: %s', cmd, query, response)

    if proc.returncode:
        raise RuntimeError('"{0}" failed with: {1}'.format(cmd, stderr.decode()))

    if format in ('JSON', 'JSONCompact', 'JSONEachRow'):
        return json.loads(response)

    return response


def enable_force_drop_table():
    """
    Touch force_drop_table ClickHouse flag.
    """
    Path('/var/lib/clickhouse/flags/force_drop_table').touch()


if __name__ == '__main__':
    main()
