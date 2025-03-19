# -*- coding: utf-8 -*-

# Находит облака, которые должны быть удалены после длительной блокировки.

import argparse
import itertools
import logging
import time

from concurrent.futures import TimeoutError

import ydb
from ydb import BaseRequestSettings

import colorama
from colorama import Back, Style, Cursor
from datetime import datetime


CLOUDS_TABLE = 'clouds'
CLOUDS_HISTORY_TABLE = 'clouds_history'
BLOCKED_STATUSES = {'BLOCKED', 'BLOCKED_BY_BILLING', 'BLOCKED_MANUALLY'}
INACTIVE_STATUSES = {'CREATING', *BLOCKED_STATUSES}

FIX_DATE = 1555722000


def spinning_cursor():
    while True:
        for cursor in '|/-\\':
            yield cursor


def grouper(iterable, n):
    yield from itertools.zip_longest(*[iter(iterable)] * n)


def log_level_from_verbose(verbose):
    if verbose > 1:
        return logging.DEBUG
    if verbose > 0:
        return logging.INFO

    return logging.ERROR


def main():
    colorama.init()

    parser = argparse.ArgumentParser()
    parser.add_argument('-a', '--auth-token', help='File with auth token')
    parser.add_argument('-d', '--database', help='Name of the database')
    parser.add_argument('-e', '--endpoint', help='HOST[:PORT]', default='localhost:2135')
    parser.add_argument('-l', '--limit', help='Maximum number of clouds to delete', default=10000, type=int)
    parser.add_argument('-o', '--out', help='Output file', required=True)
    parser.add_argument('-p', '--period', help='Period of non-use (days)', default=60, type=int)
    parser.add_argument('-r', '--table-prefix', required=True)
    parser.add_argument('-s', '--statuses', help='Inactive statuses', default=INACTIVE_STATUSES, nargs='+', choices=INACTIVE_STATUSES)
    parser.add_argument('-t', '--timeout', help='Ydb request timeout (seconds)', default=3600, type=int)
    parser.add_argument('-v', '--verbose', help='Verbose output', action='count', default=0)

    args = parser.parse_args()

    clouds = dict()
    n_clouds = 0
    creating_clouds = 0
    active_clouds = 0
    blocked_clouds = 0
    pending_deletion = 0
    deleting_clouds = 0
    deleted_clouds = 0

    logger = logging.getLogger(__name__)
    logger.setLevel(log_level_from_verbose(args.verbose))
    logger.addHandler(logging.StreamHandler())

    if log_level_from_verbose(args.verbose) <= logging.INFO:
        ydb_logger = logging.getLogger('ydb')
        ydb_logger.setLevel(logging.INFO)
        ydb_logger.addHandler(logging.StreamHandler())

    auth_token = None
    if args.auth_token:
        with open(args.auth_token, 'r') as token_file:
            auth_token = token_file.read()

    driver_config = ydb.DriverConfig(args.endpoint, args.database, auth_token=auth_token)
    with ydb.Driver(driver_config) as driver:
        try:
            driver.wait(timeout=5)
        except TimeoutError:
            raise RuntimeError('Connect failed to YDB')

        with ydb.SessionPool(driver, size=5) as session_pool:
            spinner = spinning_cursor()
            loaded = 0

            def print_loading(entity, loaded, done):
                print(Cursor.UP() + Style.BRIGHT + '[' + ('+' if done else next(spinner)) + '] Loading ' + entity + ' ('
                        + str(loaded) + ' rows loaded)' + Style.RESET_ALL)

            print()
            print()
            print_loading(CLOUDS_TABLE, 0, False)

            it = session_pool.retry_operation_sync(lambda session: session.read_table(args.table_prefix + CLOUDS_TABLE,
                                                                                      columns=['id', 'status'],
                                                                                      settings=BaseRequestSettings().with_timeout(args.timeout)))
            while True:
                try:
                    data_chunk = next(it)
                except StopIteration:
                    break

                for row in data_chunk.rows:
                    id = row[0]
                    status = row[1]

                    logger.debug("{}: {}".format(id, status))

                    if status in args.statuses:
                        clouds[id] = {'id': id, 'status': status, 'last_modified': None, 'last_inactive': None}

                    if status in BLOCKED_STATUSES:
                        blocked_clouds += 1
                    elif status == 'ACTIVE':
                        active_clouds += 1
                    elif status == 'PENDING_DELETION':
                        pending_deletion += 1
                    elif status == 'DELETING':
                        deleting_clouds += 1
                    elif status == 'DELETED':
                        deleted_clouds += 1
                    elif status == 'CREATING':
                        creating_clouds += 1
                    else:
                        raise RuntimeError('Unknown cloud status ' + status + "(" + id + ")")

                loaded += len(data_chunk.rows)
                print_loading(CLOUDS_TABLE, loaded, False)

            print_loading(CLOUDS_TABLE, loaded, True)
            n_clouds = loaded

            spinner = spinning_cursor()
            loaded = 0

            print()
            print_loading(CLOUDS_HISTORY_TABLE, 0, False)

            it = session_pool.retry_operation_sync(lambda session: session.read_table(args.table_prefix + CLOUDS_HISTORY_TABLE,
                                                                                      columns=['id', 'deleted_at', 'modified_at', 'status'],
                                                                                      ordered=True,
                                                                                      settings=BaseRequestSettings().with_timeout(args.timeout)))
            while True:
                try:
                    data_chunk = next(it)
                except StopIteration:
                    break

                for row in data_chunk.rows:
                    id = row[0]
                    deleted_at = row[1]
                    modified_at = int(row[2])
                    status = row[3]

                    if id in clouds:
                        cloud = clouds[id]
                        # Облако было удалено во время работы программы
                        if deleted_at is not None or status == 'DELETED':
                            del clouds[id]
                            continue

                        if cloud['last_modified'] is None:
                            cloud['last_modified'] = modified_at
                        elif cloud['last_modified'] > modified_at:
                            raise RuntimeError('Invalid cloud history ' + id)

                        if status in args.statuses:
                            if cloud['last_inactive'] is None and (cloud['status'] == 'CREATING' or modified_at > FIX_DATE):
                                cloud['last_inactive'] = modified_at
                        else:
                            cloud['last_inactive'] = None

                loaded += len(data_chunk.rows)
                print_loading(CLOUDS_HISTORY_TABLE, loaded, False)

            print_loading(CLOUDS_HISTORY_TABLE, loaded, True)

    blocked = list()

    now = int(time.time())
    for id in clouds:
        cloud = clouds[id]
        last_inactive = cloud['last_inactive']
        if last_inactive is None:
            print('\t' + Back.RED + 'Invalid cloud status ' + id + Style.RESET_ALL)
        else:
            logger.debug("Cloud {} last inactive {}".format(id, datetime.fromtimestamp(last_inactive)))
            if now - last_inactive > args.period * 24 * 3600:
                blocked.append(cloud)
                if len(blocked) >= args.limit:
                    break

    print()
    print('Total clouds       ' + str(n_clouds))
    print('\tBLOCKED          ' + str(blocked_clouds))
    print('\tACTIVE           ' + str(active_clouds))
    print('\tPENDING_DELETION ' + str(pending_deletion))
    print('\tDELETING         ' + str(deleting_clouds))
    print('\tDELETED          ' + str(deleted_clouds))
    print('\tCREATING         ' + str(creating_clouds))
    print()
    print('Inactive clouds ' + str(len(clouds)))
    print('Clouds to delete ' + str(len(blocked)))

    with open(args.out, 'w') as out_file:
        for c in sorted(blocked, key=lambda x: x['last_inactive']):
            print(str(c), file=out_file)


if __name__ == "__main__":
    main()
