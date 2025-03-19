# -*- coding: utf-8 -*-

# Копирует таблицу из одной базы данных в другую. Поля у таблиц должны совпадать.

import argparse
import itertools
import logging

from concurrent.futures import TimeoutError

import ydb
from ydb import BaseRequestSettings

import colorama
from colorama import Style, Cursor


def spinning_cursor():
    while True:
        for cursor in '|/-\\':
            yield cursor


def grouper(iterable, n):
    yield from itertools.zip_longest(*[iter(iterable)] * n)


def make_columns_mapping(columns, src_columns, dst_columns):
    mapping = {}
    for column in columns:
        delimiter = column.rfind(" as ")
        [src, dst] = [column[:delimiter].strip(), column[delimiter+4:].strip()] if delimiter >= 0 else [column, column]
        if dst not in dst_columns:
            raise RuntimeError('The destination table does not contain column {}'.format(dst))
        if src[0] != "'" and src not in src_columns:
            raise RuntimeError('The source table does not contain column {}'.format(src))
        mapping[dst] = src
    return mapping


def format_column_name(name):
    return name if name[0] == "'" else 'T.`' + name + '`'


def make_select_query(src_table, src_desc, columns_mapping):
    src_columns = {c.name: str(c.type.item) for c in src_desc.columns if c.name in columns_mapping.values()}

    select_query = "DECLARE $Input AS 'List<Struct<"
    select_query += ','.join(map(lambda c: c + ':' + src_columns[c], src_desc.primary_key))
    select_query += ">>';"
    select_query += 'SELECT '
    select_query += ','.join(map(lambda c: format_column_name(columns_mapping[c]) + ' AS `' + c + '`', columns_mapping))
    select_query += ' FROM AS_TABLE($Input) AS K JOIN [{}] AS T ON '.format(src_table)
    select_query += ' AND '.join(map(lambda c: 'K.`' + c + '` = T.`' + c + '`', src_desc.primary_key))

    return select_query


def make_replace_query(dst_table, dst_desc, columns_mapping):
    dst_columns = {c.name: str(c.type.item) for c in dst_desc.columns if c.name in columns_mapping}

    replace_query = "DECLARE $Input AS 'List<Struct<"
    replace_query += ','.join(map(lambda c: c + ':' + str(dst_columns[c]) + '?', dst_columns))
    replace_query += ">>';"
    replace_query += 'UPSERT INTO [{}] SELECT * FROM AS_TABLE($Input)'.format(dst_table)

    return replace_query


def make_delete_query(dst_table, dst_desc):
    dst_columns = {c.name: str(c.type.item) for c in dst_desc.columns}

    delete_query = "DECLARE $Input AS 'List<Struct<"
    delete_query += ','.join(map(lambda c: c + ':' + dst_columns[c] + '?', dst_desc.primary_key))
    delete_query += ">>';"
    delete_query += 'DELETE FROM [{}] ON SELECT * FROM AS_TABLE($Input)'.format(dst_table)

    return delete_query


def check_truncated(result):
    if result.truncated:
        raise RuntimeError(
            'Got only {} rows from the DB. Please specify a lower batch size.'.format(len(result.rows)))
    return result.rows


def add_rows(src_session, dst_session, chunk, select_query, replace_query):
    select = src_session.prepare(select_query)
    replace = dst_session.prepare(replace_query)

    src_tx = src_session.transaction(ydb.SerializableReadWrite()).begin()
    dst_tx = dst_session.transaction(ydb.SerializableReadWrite()).begin()

    res = src_tx.execute(select, {'$Input': chunk})
    rows = check_truncated(res[0])

    if len(rows) != 0:
        dst_tx.execute(replace, {'$Input': rows})

    src_tx.commit()
    dst_tx.commit()

    return len(rows)


def to_pk_tuple(row, pk):
    key = list()
    for k in pk:
        key.append(row[k])
    return tuple(key)


def remove_rows(src_session, dst_session, chunk, select_query, delete_query, src_pk):
    select = src_session.prepare(select_query)
    delete = dst_session.prepare(delete_query)

    src_tx = src_session.transaction(ydb.SerializableReadWrite()).begin()
    dst_tx = dst_session.transaction(ydb.SerializableReadWrite()).begin()

    res = src_tx.execute(select, {'$Input': chunk})
    rows = check_truncated(res[0])

    result = 0

    if len(rows) != len(chunk):
        dst_rows = {to_pk_tuple(r, src_pk): r for r in chunk}
        src_pks = set(to_pk_tuple(r, src_pk) for r in rows)

        diff = set(dst_rows.keys()).difference(src_pks)
        diff_rows = list(dst_rows[pk] for pk in diff)

        assert len(diff_rows) == len(chunk) - len(rows)

        dst_tx.execute(delete, {'$Input': diff_rows})

        result = len(diff_rows)

    src_tx.commit()
    dst_tx.commit()

    return result


def read_token_from_file(token_file_name):
    auth_token = None
    if token_file_name:
        with open(token_file_name, 'r') as token_file:
            auth_token = token_file.read().strip()
    return auth_token


def main():
    colorama.init()

    parser = argparse.ArgumentParser()
    parser.add_argument('-b', '--batch-size', help='Max rows to proceed', default=1000, type=int)
    parser.add_argument('-c', '--column', help='Column to proceed', action='append')
    parser.add_argument('-t', '--timeout', help='Ydb request timeout', default=3600, type=int)
    parser.add_argument('-v', '--verbose', default=False, action='store_true')
    parser.add_argument('-o', '--only-upsert', help='Only upsert data', default=False, action='store_true')
    parser.add_argument('--src-endpoint', help='Source HOST[:PORT]', default='localhost:2135')
    parser.add_argument('--src-database', help='Name of the source database')
    parser.add_argument('--src-auth-token', help='File with auth token for the source database')
    parser.add_argument('--dst-endpoint', help='Destination HOST[:PORT]', default='localhost:2135')
    parser.add_argument('--dst-database', help='Name of the destination database')
    parser.add_argument('--dst-auth-token', help='File with auth token for the destination database')
    parser.add_argument('src', help='Source table')
    parser.add_argument('dst', help='Destination table')

    args = parser.parse_args()

    if args.verbose:
        logger = logging.getLogger('ydb')
        logger.setLevel(logging.INFO)
        logger.addHandler(logging.StreamHandler())

    print('Copying ' + Style.BRIGHT + args.src_endpoint + ' ' + args.src + Style.RESET_ALL +
              ' => ' + Style.BRIGHT + args.dst_endpoint + ' ' + args.dst + Style.RESET_ALL)

    src_auth_token = read_token_from_file(args.src_auth_token)
    src_driver_config = ydb.DriverConfig(args.src_endpoint, args.src_database, auth_token=src_auth_token)

    dst_auth_token = read_token_from_file(args.dst_auth_token)
    dst_driver_config = ydb.DriverConfig(args.dst_endpoint, args.dst_database, auth_token=dst_auth_token)
    request_settings=BaseRequestSettings() \
               .with_timeout(args.timeout) \
               .with_cancel_after(args.timeout) \
               .with_operation_timeout(args.timeout)

    with ydb.Driver(src_driver_config) as src_driver, ydb.Driver(dst_driver_config) as dst_driver:
        try:
            src_driver.wait(timeout=5)
            dst_driver.wait(timeout=5)
        except TimeoutError as e:
            raise RuntimeError('Connect failed to YDB: {!r}'.format(e))

        with ydb.SessionPool(src_driver, size=5) as src_session_pool, ydb.SessionPool(dst_driver, size=5) as dst_session_pool:
            src_desc = src_session_pool.retry_operation_sync(lambda session: session.describe_table(args.src))
            dst_desc = dst_session_pool.retry_operation_sync(lambda session: session.describe_table(args.dst))

            dst_columns = set(c.name for c in dst_desc.columns)
            src_columns = set(c.name for c in src_desc.columns)
            columns_mapping = None

            if args.column:
                columns_mapping = make_columns_mapping(args.column, src_columns, dst_columns)
                if not set(dst_desc.primary_key).issubset(columns_mapping.keys()):
                    raise RuntimeError('The destination table column(s) {} should be selected'.format(dst_desc.primary_key))
                if not set(src_desc.primary_key).issubset(columns_mapping.values()):
                    raise RuntimeError('The source table column(s) {} should be selected'.format(src_desc.primary_key))
            else:
                if src_columns != dst_columns:
                    raise RuntimeError('The table columns do not match')
                columns_mapping = {c: c for c in src_columns}

            select_query = make_select_query(args.src, src_desc, columns_mapping)
            replace_query = make_replace_query(args.dst, dst_desc, columns_mapping)
            delete_query = make_delete_query(args.dst, dst_desc)

            def delete():
                spinner = spinning_cursor()
                read = 0
                deleted = 0

                def print_deleting(read, deleted, done):
                    print(Cursor.UP() + Style.BRIGHT
                          + '[' + ('+' if done else next(spinner)) + '] ' + 'Deleting rows from the destination table ('
                          + str(read) + ' rows processed, ' + str(deleted) + ' rows deleted)' + Style.RESET_ALL)

                print()
                print()
                print_deleting(0, 0, False)
                it = dst_session_pool.retry_operation_sync(
                    lambda session: session.read_table(args.dst, settings=request_settings))
                while True:
                    try:
                        data_chunk = next(it)
                    except StopIteration:
                        break

                    for c in grouper(data_chunk.rows, args.batch_size):
                        c = list(filter(None, c))
                        d = src_session_pool.retry_operation_sync(
                            lambda src_session: dst_session_pool.retry_operation_sync(
                                lambda dst_session: remove_rows(
                                    src_session, dst_session, c, select_query, delete_query, src_desc.primary_key)))
                        read += len(c)
                        deleted += d

                        print_deleting(read, deleted, False)
                print_deleting(read, deleted, True)

            def upsert():
                spinner = spinning_cursor()
                read = 0
                updated = 0

                def print_updating(read, updated, done):
                    print(Cursor.UP() + Style.BRIGHT + '[' + ('+' if done else next(spinner))
                          + '] Upserting rows to the destination table ('
                          + str(read) + ' rows processed, ' + str(updated) + ' rows upserted)' + Style.RESET_ALL)

                print()
                print_updating(0, 0, False)
                it = src_session_pool.retry_operation_sync(
                    lambda session: session.read_table(args.src, settings=request_settings))
                while True:
                    try:
                        data_chunk = next(it)
                    except StopIteration:
                        break

                    for c in grouper(data_chunk.rows, args.batch_size):
                        c = list(filter(None, c))
                        u = src_session_pool.retry_operation_sync(
                            lambda src_session: dst_session_pool.retry_operation_sync(
                                lambda dst_session: add_rows(src_session, dst_session, c, select_query, replace_query)))
                        read += len(c)
                        updated += u

                        print_updating(read, updated, False)
                print_updating(read, updated, True)

            # Удаление можно отключить параметром --only-upsert
            if not args.only_upsert:
                delete()
            upsert()

    print()
    print('The table ' + Style.BRIGHT + args.dst_endpoint + ' ' + args.dst + Style.RESET_ALL + ' is ready for use')


if __name__ == "__main__":
    main()
