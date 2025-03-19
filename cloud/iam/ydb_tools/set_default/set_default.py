# -*- coding: utf-8 -*-

# Заполняет добавленные в таблицу столбцы заданным значением.
#
# В значениях столбцов нужно указывать Python-выражения.
#
# Поддерживается замена значений столбцов первичного ключа через DELETE-UPSERT.
#
# Пример запуска
# $ set_default my_table --column name '"test"' --column value 42 --column flag true
#
# TODO
# 1. Получать из describe_table диапазоны ключей для шардов и распараллелить процесс создания индекса.
# 2. Иметь возможность прерывать скрипт и продолжать выполнение с предыдущего места.

import argparse
import logging

from concurrent.futures import TimeoutError

import ydb
from ydb import BaseRequestSettings

from itertools import zip_longest, chain

from pathlib import Path

import colorama
from colorama import Back, Style, Cursor

from queries import make_update_query, make_replace_query, replace_columns, update_columns, get_update_input


def spinning_cursor():
    while True:
        for cursor in '|/-\\':
            yield cursor


def grouper(iterable, n):
    yield from zip_longest(*[iter(iterable)] * n)


def main():
    colorama.init()

    parser = argparse.ArgumentParser()
    parser.add_argument('-e', '--endpoint', help='HOST[:PORT]', default='localhost:2135')
    parser.add_argument('-d', '--database', help='Name of the database')
    parser.add_argument('-a', '--auth-token', help='Auth token file (default: ~/.ydb/token, if exists)')
    parser.add_argument('-b', '--batch-size', help='Max rows to proceed', default=1000, type=int)
    parser.add_argument('-t', '--timeout', help='Ydb request timeout', default=3600, type=int)
    parser.add_argument('-c', '--column', help='Update column with the result of evaluating a Python expression.'
                        + ' Access row data using "row" variable (by default contains primary key columns only)',
                        nargs=2, action='append', type=str, default=[], metavar=('COLUMN', 'EXPRESSION'))
    parser.add_argument('-f', '--fetch-all-columns', help='Fetch all columns to use in "row" variable',
                        action='store_true', default=False)
    parser.add_argument('-n', '--dry-run', help='Do not update table, read and eval only', action='store_true', default=False)
    parser.add_argument('-v', '--verbose', default=False, action='store_true')
    parser.add_argument('table', help='Table')

    args = parser.parse_args()

    if args.verbose:
        logger = logging.getLogger('ydb')
        logger.setLevel(logging.INFO)
        logger.addHandler(logging.StreamHandler())

    if args.dry_run:
        print('Performing a DRY RUN for table ' + Style.BRIGHT + args.table + Style.RESET_ALL)
    else:
        print('Updating table ' + Style.BRIGHT + args.table + Style.RESET_ALL)

    auth_token = None
    default_token_path = Path.home().joinpath(".ydb/token")
    token_path = args.auth_token or (default_token_path if default_token_path.exists() else None)
    if token_path:
        with open(token_path, 'r') as token_file:
            auth_token = token_file.read().strip()

    request_settings=BaseRequestSettings() \
               .with_timeout(args.timeout) \
               .with_cancel_after(args.timeout) \
               .with_operation_timeout(args.timeout)
    driver_config = ydb.DriverConfig(args.endpoint, args.database, auth_token=auth_token)
    with ydb.Driver(driver_config) as driver:
        try:
            driver.wait(timeout=5)
        except TimeoutError:
            raise RuntimeError('Connect failed to YDB')

        with ydb.SessionPool(driver, size=5) as session_pool:
            desc = session_pool.retry_operation_sync(lambda session: session.describe_table(args.table))

            all_columns = set(c.name for c in desc.columns)
            columns = dict(args.column)
            compiled_columns = dict()
            updating_pk = False

            for c in columns:
                if c in desc.primary_key:
                    if not args.fetch_all_columns:
                        raise RuntimeError('Updating primary key requires `--fetch-all-columns` flag')
                    updating_pk = True
                if c not in all_columns:
                    raise RuntimeError('There is no column `' + c + '` in the table `' + args.table + '`')
                try:
                    compiled_columns[c] = compile(columns[c], '<evaluated expression>', 'eval')
                except SyntaxError:
                    raise RuntimeError('Syntax error in Python expression `' + columns[c] + '`')

            def format_column_value(val):
                return "eval( " + val[:90] + (val[90:] and "...") + " )"

            print()
            print('Table columns')
            for c in chain(desc.primary_key, (c.name for c in desc.columns if c.name not in desc.primary_key)):
                if c in columns:
                    if c in desc.primary_key:
                        print('\t' + Back.RED + c + Style.RESET_ALL, end='')
                    else:
                        print('\t' + Back.YELLOW + c + Style.RESET_ALL, end='')
                    print(Style.BRIGHT + ' -> ' + Style.RESET_ALL + format_column_value(columns[c]))
                elif c in desc.primary_key:
                    print('\t' + Back.BLUE + c + Style.RESET_ALL)
                else:
                    print('\t' + c)

            spinner = spinning_cursor()
            read = 0

            def print_updating(read, done):
                action = 'Evaluating' if args.dry_run else 'Updating'
                print(Cursor.UP() + Style.BRIGHT + '[' + ('+' if done else next(spinner)) + '] ' + action + ' rows ('
                      + str(read) + ' rows processed)' + Style.RESET_ALL)

            print()
            print()
            print_updating(0, False)

            read_table_columns = all_columns if args.fetch_all_columns else set(desc.primary_key).union(columns.keys())
            it = session_pool.retry_operation_sync(
                lambda session: session.read_table(
                    args.table,
                    columns=read_table_columns,
                    settings=request_settings
                )
            )

            if updating_pk:
                replace_query = make_replace_query(args.table, desc)
                def update_method(session):
                    replace_columns(session, c, replace_query, desc, compiled_columns)
            else:
                update_query = make_update_query(args.table, desc, columns)
                def update_method(session):
                    update_columns(session, c, update_query, desc, compiled_columns)

            while True:
                try:
                    data_chunk = next(it)
                except StopIteration:
                    break

                for c in grouper(data_chunk.rows, args.batch_size):
                    c = list(filter(None, c))
                    if args.dry_run:
                        get_update_input(c, desc, compiled_columns)
                    else:
                        session_pool.retry_operation_sync(update_method)
                    read += len(c)

                    print_updating(read, False)

            print_updating(read, True)

    print()
    if args.dry_run:
        print('Dry run for table ' + args.table + ' completed')
    else:
        print('The table ' + args.table + ' is ready for use')


if __name__ == "__main__":
    main()
