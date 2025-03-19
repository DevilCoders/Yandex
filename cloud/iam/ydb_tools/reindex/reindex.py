# -*- coding: utf-8 -*-

# Наполняет данными индексную таблицу. Также можно использовать для починки индекса.
#
# Перед запуском скрипта в приложениях, модифицирующих основную таблицу, должна быть настроена модификация индекса.
# Во время выкатки существует момент, когда одновременно работают приложения с включенной модификацией индекса при
# модификациях в основной таблице (новая версия) и без нее (старая версия). Скрипт нужно запускать, когда старых
# версий приложения уже не осталось.
#
# После запуска скрипта приложения могут использовать индексную таблицу для выборок.
#
# Скрипт гарантирует консистентность индекса в момент обновления, его можно использовать, если таблица уже используется
# для выборок.
#
# Обязательным условием для индексной таблицы является наличие в ней полей, составляющих PK основной. При этом общее
# количество полей индексной таблицы может быть меньше, чем в основной.
# В PK основной таблицы не должно быть NULL значений (в PK индекса могут быть).
#
# Скрипт исправляет ошибки в индексе, которые возникают по следующим причинам:
# 1. Данные были добавлены старой версией приложения. В этом случае в индексной таблице нет записи.
# 2. Данные были добавлены или изменены новой версией приложения, затем эти же данные были изменены старой версией. В
#    этом случае данные в записи в индексной таблице отличаются от данных в основной.
# 3. Данные были добавлены новой версией приложения, затем удалены старой версией. В этом случае в индексной таблице
#    осталась запись, которая уже пропала из основной таблицы.
#
# TODO
# 1. Получать из describe_table диапазоны ключей для шардов и распараллелить процесс создания индекса.
# 2. Иметь возможность прерывать скрипт и продолжать выполнение с предыдущего места.
# 3. dry-run
# 4. В read_table читать только те столбцы, которые участвуют дальше в запросах.

import argparse
import itertools
import logging

from concurrent.futures import TimeoutError

import ydb
from ydb import BaseRequestSettings

import colorama
from colorama import Back, Style, Cursor


def spinning_cursor():
    while True:
        for cursor in '|/-\\':
            yield cursor


def grouper(iterable, n):
    yield from (filter(None, c) for c in itertools.zip_longest(*[iter(iterable)] * n))


def yql_syntax(query):
    return '--!syntax_v1\n' + query


def make_select_query(src_table, src_desc, extra_columns):
    src_columns = {c.name: str(c.type.item) for c in src_desc.columns}

    select_query = 'DECLARE $Input AS List<Struct<'
    select_query += ','.join(map(lambda c: c + ':' + src_columns[c], src_desc.primary_key))
    if extra_columns:
        select_query += ','
        select_query += ','.join(map(lambda c: c + ':' + src_columns[c] + '?', extra_columns))
    select_query += '>>;\n'
    select_query += 'SELECT t.*'
    select_query += ' FROM AS_TABLE($Input) AS k JOIN `{}` AS t ON '.format(src_table)
    select_query += ' AND '.join(map(lambda c: 't.`{0}` = k.`{0}`'.format(c), src_desc.primary_key))
    if extra_columns:
        select_query += ' WHERE '
        select_query += ' AND '.join(map(lambda c: 't.`{0}` IS NOT DISTINCT FROM k.`{0}`'.format(c), extra_columns))

    return yql_syntax(select_query)


def make_replace_query(idx_table, idx_desc):
    replace_query = 'DECLARE $Input AS List<Struct<'
    replace_query += ','.join(map(lambda c: c.name + ':' + str(c.type.item) + '?', idx_desc.columns))
    replace_query += '>>;\n'
    replace_query += 'UPSERT INTO `{}` SELECT * FROM AS_TABLE($Input)'.format(idx_table)

    return yql_syntax(replace_query)


def make_delete_query(idx_table, idx_desc):
    idx_columns = {c.name: str(c.type.item) for c in idx_desc.columns}

    delete_query = 'DECLARE $Input AS List<Struct<'
    delete_query += ','.join(map(lambda c: c + ':' + idx_columns[c] + '?', idx_desc.primary_key))
    delete_query += '>>;\n'
    delete_query += 'DELETE FROM `{}` ON SELECT * FROM AS_TABLE($Input)'.format(idx_table)

    return yql_syntax(delete_query)


def check_truncated(result):
    if result.truncated:
        raise RuntimeError(
            'Got only {} rows from the DB. Please specify a lower batch size.'.format(len(result.rows)))
    return result.rows


def add_to_index(session, chunk, select_query, replace_query):
    select = session.prepare(select_query)
    replace = session.prepare(replace_query)

    tx = session.transaction(ydb.SerializableReadWrite()).begin()

    # Важно, чтобы в одной транзакции было чтение данных из основной таблицы и приведение в консистентное состояние
    # части индекса для них.
    res = tx.execute(select, {'$Input': chunk})
    rows = check_truncated(res[0])

    if len(rows) != 0:
        tx.execute(replace, {'$Input': rows})

    tx.commit()

    return len(rows)


def is_pk_has_null(row, pk):
    return any(row[c] is None for c in pk)


# Ищет строку row среди строк rows, считая равными строки с одинаковыми значениями в столбцах, которые присутствуют
# одновременно в обеих строках.
def find_row(row, rows):
    return any(all(k not in r or r[k] == v for k, v in row.items()) for r in rows)


def remove_from_index(session, chunk, select_query, delete_query, skip_null_pk, idx_pk):
    select = session.prepare(select_query)
    delete = session.prepare(delete_query)

    tx = session.transaction(ydb.SerializableReadWrite()).begin()

    # Важно, чтобы в одной транзакции было чтение данных из основной таблицы и приведение в консистентное состояние
    # части индекса для них.
    res = tx.execute(select, {'$Input': chunk})
    rows = check_truncated(res[0])

    # Отбираем ряды на удаление
    absent_rows = list(filter(lambda row: (skip_null_pk and is_pk_has_null(row, idx_pk)) or not find_row(row, rows), chunk))

    if absent_rows:
        tx.execute(delete, {'$Input': absent_rows})

    tx.commit()

    return len(absent_rows)


def main():
    colorama.init()

    parser = argparse.ArgumentParser()
    parser.add_argument('-e', '--endpoint', help='HOST[:PORT]', default='localhost:2135')
    parser.add_argument('-d', '--database', help='Name of the database')
    parser.add_argument('-a', '--auth-token', help='File with auth token')
    parser.add_argument('-b', '--batch-size', help='Max rows to proceed', default=1000, type=int)
    parser.add_argument('-t', '--timeout', help='Ydb request timeout', default=3600, type=int)
    parser.add_argument('-v', '--verbose', default=False, action='store_true')
    parser.add_argument('--delete-first', help='Perform delete, then upsert', default=False, action='store_true')
    parser.add_argument('--skip-rows-with-null-in-pk', help='Skip rows with null in the primary key of the index table',
                        default=False, action='store_true')
    parser.add_argument('src', help='Source table')
    parser.add_argument('idx', help='Index table')

    args = parser.parse_args()

    if args.verbose:
        logger = logging.getLogger('ydb')
        logger.setLevel(logging.INFO)
        logger.addHandler(logging.StreamHandler())

    def resolve_table_path(database, path):
        return path if (path[0] == '/') else database + ('' if database[-1] == '/' else '/') + path

    src = resolve_table_path(args.database, args.src)
    idx = resolve_table_path(args.database, args.idx)

    print('Creating index `' + Style.BRIGHT + idx + Style.RESET_ALL
          + '` for `' + Style.BRIGHT + src + Style.RESET_ALL + '`')

    auth_token = None
    if args.auth_token:
        with open(args.auth_token, 'r') as token_file:
            auth_token = token_file.read()

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
            src_desc = session_pool.retry_operation_sync(lambda session: session.describe_table(src))
            idx_desc = session_pool.retry_operation_sync(lambda session: session.describe_table(idx))

            def print_columns(desc):
                for c in desc.primary_key:
                    print('\t' + Back.BLUE + c + Style.RESET_ALL)
                for c in desc.columns:
                    if c.name not in desc.primary_key:
                        print('\t' + c.name)

            print()
            print('Source table columns')
            print_columns(src_desc)

            print()
            print('Index table columns')
            print_columns(idx_desc)

            print()

            idx_columns = set(c.name for c in idx_desc.columns)
            for c in src_desc.primary_key:
                if c not in idx_columns:
                    raise RuntimeError('The index table `{}` must contain the `{}` column from source table PK'.format(
                        idx, c))

            # Если в PK индекса имеются поля, отсутствующие в PK основной таблицы, то возможна ситуация, когда в индексе
            # сразу нескольким записям соответствует одна запись из основной таблицы. При этом максимум одна запись из
            # основной таблицы будет совпадать с какой-либо записью из индекса, а остальные будут отличаться в
            # каких-либо полях, составляющих PK индекса. Поэтому добавляем в PK основной таблицы те поля, которые
            # имеются в PK индекса, но отсутствуют в PK основной таблицы.
            #
            # Пример:
            # Основная таблица (PK parent, name)
            # id:1, parent p1, name: n1
            #
            # Индекс (PK id)
            # id:2, parent p1, name: n1
            # id:3, parent p1, name: n1
            #
            # Для обеих записей индекса есть запись в основной таблице (parent p1, name: n1) и всё работает без ошибок
            # (за исключением того, что при листинге по индексу есть задвоившиеся записи), но при удалении записи из
            # основной таблицы обе записи индекса останутся на месте и логика работы приложения может быть нарушена.
            # Если прибавить поле из PK индекса к PK основной таблицы, то выборка по-прежнему будет происходить по
            # первичному ключу, но при этом записи будут различаться и программа сможет привести индекс в соответствие.

            src_columns = set(c.name for c in src_desc.columns)
            extra_columns = list(idx_columns - set(src_desc.primary_key))
            for c in extra_columns:
                if c not in src_columns:
                    raise RuntimeError('The source table `{}` must contain the `{}` column from index table'.format(
                        src, c))

            select_query = make_select_query(src, src_desc, extra_columns)
            replace_query = make_replace_query(idx, idx_desc)
            delete_query = make_delete_query(idx, idx_desc)

            def delete():
                spinner = spinning_cursor()
                read = 0
                deleted = 0

                def print_deleting(read, deleted, done):
                    print(Cursor.UP() + Style.BRIGHT
                          + '[' + ('+' if done else next(spinner)) + '] ' + 'Deleting rows from the index ('
                          + str(read) + ' rows processed, ' + str(deleted) + ' rows deleted)' + Style.RESET_ALL)

                print()
                print_deleting(0, 0, False)
                it = session_pool.retry_operation_sync(
                    lambda session: session.read_table(idx, settings=request_settings))
                while True:
                    try:
                        data_chunk = next(it)
                    except StopIteration:
                        break

                    for chunk in grouper(data_chunk.rows, args.batch_size):
                        chunk = list(chunk)
                        d = session_pool.retry_operation_sync(
                            lambda session: remove_from_index(
                                session, chunk, select_query, delete_query,
                                args.skip_rows_with_null_in_pk, idx_desc.primary_key))
                        read += len(chunk)
                        deleted += d

                        print_deleting(read, deleted, False)
                print_deleting(read, deleted, True)
            def upsert():
                spinner = spinning_cursor()
                read = 0
                updated = 0

                def print_updating(read, updated, done):
                    print(Cursor.UP() + Style.BRIGHT + '[' + ('+' if done else next(spinner))
                          + '] Upserting rows in the index ('
                          + str(read) + ' rows processed, ' + str(updated) + ' rows upserted)' + Style.RESET_ALL)

                print()
                print_updating(0, 0, False)
                it = session_pool.retry_operation_sync(
                    lambda session: session.read_table(src, settings=request_settings))
                while True:
                    try:
                        data_chunk = next(it)
                    except StopIteration:
                        break

                    for chunk in grouper(data_chunk.rows, args.batch_size):
                        chunk = list(chunk) if not args.skip_rows_with_null_in_pk \
                            else list(filter(lambda row: not is_pk_has_null(row, idx_desc.primary_key), chunk))

                        if chunk:
                            u = session_pool.retry_operation_sync(
                                lambda session: add_to_index(session, chunk, select_query, replace_query))
                            read += len(chunk)
                            updated += u

                        print_updating(read, updated, False)
                print_updating(read, updated, True)

            if args.delete_first:
                # Сначала удаляем лишние записи из индекса, пробегая по нему, а потом добавляем в него новые.
                # В таком порядке процесс удаления пробежит меньшее количество записей в индексе.
                delete()
                upsert()
            else:
                # Сначала добавляем в индекс недостающие записи, затем удаляем дубли и лишние записи.
                # Если в основной таблице и индексе одинаковое количество записей, но одно из полей не совпадает,
                # то мы должны сначала удвоить количество записей в индексе, а затем удалить лишнее.
                upsert()
                delete()

    print()
    print('The index `' + idx + '` is ready for use')


if __name__ == "__main__":
    main()
