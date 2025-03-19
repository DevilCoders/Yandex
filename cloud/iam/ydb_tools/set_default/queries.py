# -*- coding: utf-8 -*-

import ydb


def make_update_query(table, desc, columns):
    query_columns = dict()
    for c in desc.columns:
        if c.name in desc.primary_key or c.name in columns:
            query_columns[c.name] = str(c.type.item)

    query = "DECLARE $Input AS 'List<Struct<"
    query += ','.join('{}:{}?'.format(key, value) for key, value in query_columns.items())
    query += ">>';"
    query += 'UPDATE [{}] ON SELECT * FROM AS_TABLE($Input)'.format(table)

    return query


def eval_column(columns, row):
    return {k: eval(v, {'row': row}) for (k, v) in columns.items()}


def get_update_input(chunk, desc, columns):
    return [{**{pk_col: row[pk_col] for pk_col in desc.primary_key}, **eval_column(columns, row)} for row in chunk]


def update_columns(session, chunk, update_query, desc, columns):
    replace = session.prepare(update_query)

    replace_input = get_update_input(chunk, desc, columns)

    session.transaction(ydb.SerializableReadWrite()).execute(replace, {'$Input': replace_input}, commit_tx=True)


def make_replace_query(table, desc):
    delete_columns = dict()
    upsert_columns = dict()
    for c in desc.columns:
        upsert_columns[c.name] = str(c.type.item)
        if c.name in desc.primary_key:
            delete_columns[c.name] = str(c.type.item)

    query = "DECLARE $DeleteInput AS 'List<Struct<"
    query += ','.join('{}:{}?'.format(key, value) for key, value in delete_columns.items())
    query += ">>';\n"
    query += "DECLARE $UpsertInput AS 'List<Struct<"
    query += ','.join('{}:{}?'.format(key, value) for key, value in upsert_columns.items())
    query += ">>';\n"

    query += 'DELETE FROM [{}] ON SELECT * FROM AS_TABLE($DeleteInput);\n'.format(table)
    query += 'UPSERT INTO [{}] SELECT * FROM AS_TABLE($UpsertInput);'.format(table)

    return query


def replace_columns(session, chunk, replace_query, desc, columns):
    replace = session.prepare(replace_query)

    delete_input = []
    upsert_input = []

    for row in chunk:
        pk = {pk_col: row[pk_col] for pk_col in desc.primary_key}
        upsert_row = {**row, **eval_column(columns, row)}
        pk_mod = {pk_col: upsert_row[pk_col] for pk_col in desc.primary_key}
        if pk != pk_mod:
            delete_input.append(pk)
            upsert_input.append(upsert_row)

    if len(delete_input) == 0:
        return

    session.transaction(ydb.SerializableReadWrite()).execute(
        replace,
        {'$DeleteInput': delete_input, '$UpsertInput': upsert_input},
        commit_tx=True
    )
