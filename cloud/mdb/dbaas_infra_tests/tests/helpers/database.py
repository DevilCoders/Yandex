"""
Database helpers
"""

import os

import psycopg2

from .docker import get_container, get_exposed_port


def pg_connect(context, srv_name):
    """
    Create connection to specified pg service
    """
    db_opts = context.conf['projects'][srv_name]['db']
    host_short = '{srv_name}01'.format(srv_name=srv_name)
    host, port = get_exposed_port(
        get_container(context, host_short), context.conf['projects'][srv_name][host_short]['expose']['pgbouncer'])

    dsn = ('host={host} port={port} dbname={dbname} '
           'user={user} password={password} sslmode=require').format(
               host=host, port=port, dbname=db_opts['dbname'], user=db_opts['user'], password=db_opts['password'])

    return psycopg2.connect(dsn)


def pg_dump_csv(context, srv_name, path):
    """
    Dump all user tables in the format of csv files
    """
    with pg_connect(context, srv_name) as conn:
        cur = conn.cursor()
        cur.execute("""
            SELECT schemaname,tablename
            FROM pg_tables
            WHERE schemaname
            NOT IN ('pg_catalog', 'information_schema');
            """)
        schemas_tables = cur.fetchall()

        for schema, table in schemas_tables:
            schema_dir = os.path.join(path, schema)
            os.makedirs(schema_dir, exist_ok=True)

            try:
                sql = 'COPY {0}.{1} TO STDOUT WITH CSV HEADER'.format(schema, table)
                with open(os.path.join(schema_dir, table + '.csv'), 'wb') as file:
                    cur.copy_expert(sql, file)
            except Exception as exc:
                print(f"Unable to dump {schema}.{table}: {exc}")
