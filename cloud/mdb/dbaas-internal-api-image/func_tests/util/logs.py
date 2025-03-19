"""
Logs-related functions
"""
import os

import psycopg2
import tenacity
from psycopg2 import Error


@tenacity.retry(
    retry=tenacity.retry_if_exception_type(Error),
    wait=tenacity.wait_random_exponential(multiplier=5, max=5),
    stop=tenacity.stop_after_attempt(3),
)
def save_logs(context):
    """
    Save logs and metadb content
    """
    base = context.logs_root / 'metadb'
    os.makedirs(base, exist_ok=True)
    if hasattr(context, 'metadb_container'):
        with open(base / 'docker.log', 'wb') as out:
            out.write(context.metadb_container.logs(stdout=True, stderr=True, timestamps=True))

    with psycopg2.connect(context.metadb_dsn) as conn:
        cur = conn.cursor()
        cur.execute(
            """
            SELECT schemaname, tablename
            FROM pg_tables
            WHERE schemaname IN ('dbaas')
            """
        )
        tables = cur.fetchall()

        for schema, table in tables:
            schema_path = base / schema
            os.makedirs(schema_path, exist_ok=True)
            query = 'COPY {schema}.{table} TO STDOUT WITH CSV HEADER'.format(schema=schema, table=table)
            table_file = '{name}.csv'.format(name=table)
            with open(schema_path / table_file, 'w') as out:
                cur.copy_expert(query, out)
