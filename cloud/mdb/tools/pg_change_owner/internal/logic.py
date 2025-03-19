import logging
from typing import NamedTuple

import psycopg2


def get_connection(connection_string: str):
    return psycopg2.connect(connection_string)


def _query_executer(connection):
    cur = connection.cursor()

    def exec_and_commit(statement):
        logging.info(statement)
        cur.execute(statement)
        connection.commit()

    return exec_and_commit


def _change_common(exec, db_name: str, owner: str):
    exec(f'ALTER DATABASE {db_name} OWNER TO {owner}')
    exec(f'ALTER schema public OWNER TO {owner}')
    exec(f'ALTER TABLE public.schema_version OWNER TO {owner}')
    exec(f'ALTER TYPE public.schema_version_type OWNER TO {owner}')


object_types = '{type,table,table,view,sequence,materialized view}'
object_classes = '{c,t,r,v,S,m}'

TYPE_TO_CLASS = (
    ('type', 'c'),
    ('table', 't'),
    ('table', 'r'),
    ('view', 'v'),
    ('sequence', 'S'),
    ('materialized view', 'm'),
)


class DBObject(NamedTuple):
    namespace: str
    relation: str


class FunctionObject(NamedTuple):
    namespace: str
    relation: str
    args: str


def _select_objects(connection, schema: str, kind: str) -> list[DBObject]:
    cur = connection.cursor()
    cur.execute(
        '''
          select n.nspname, c.relname
          from pg_class c, pg_namespace n
          where n.oid = c.relnamespace
            and nspname =  %s
            and relkind =  %s
            ''',
        (
            schema,
            kind,
        ),
    )
    result = []
    for row in cur.fetchall():
        result.append(DBObject(namespace=row[0], relation=row[1]))
    return result


def _select_functions(connection, schema: str) -> list[FunctionObject]:
    cur = connection.cursor()
    cur.execute(
        '''
        select n.nspname, p.proname,
           pg_catalog.pg_get_function_identity_arguments(p.oid) args
        from pg_catalog.pg_namespace n
           join pg_catalog.pg_proc p
           on p.pronamespace = n.oid
        where n.nspname = %s
    ''',
        (schema,),
    )
    result = []
    for row in cur.fetchall():
        result.append(FunctionObject(namespace=row[0], relation=row[1], args=row[2]))
    return result


def _alter_object_owner(exec, object_type: str, namespace: str, relation: str, owner: str):
    sql = f'''ALTER {object_type} {namespace}.{relation} OWNER TO {owner}'''
    exec(sql)


def _alter_function_owner(exec, object_type: str, namespace: str, relation: str, args: str, owner: str):
    sql = f'''ALTER {object_type} {namespace}.{relation}({args}) OWNER TO {owner}'''
    exec(sql)


def _change_in_schema(connection, db_name: str, schema_name: str, owner: str):
    executer = _query_executer(connection)
    executer(f'ALTER SCHEMA {schema_name} OWNER TO {owner}')
    for object_type, object_kind in TYPE_TO_CLASS:
        for db_object in _select_objects(connection, schema_name, object_kind):
            _alter_object_owner(executer, object_type, schema_name, db_object.relation, owner)
        for func_object in _select_functions(connection, schema_name):
            _alter_function_owner(executer, 'FUNCTION', schema_name, func_object.relation, func_object.args, owner)


def change_owner_in_all_objects(connection, db_name: str, schemas: list[str], owner: str):
    executer = _query_executer(connection)
    _change_common(executer, db_name, owner)
    for schema_name in schemas:
        _change_in_schema(connection, db_name, schema_name, owner)


def execute_extra(connection, extra_queries: list[str]):
    executer = _query_executer(connection)
    for query in extra_queries:
        executer(query)
