import os

import psycopg2


def test_with_simple_select():
    host = os.environ.get('DBM_POSTGRESQL_RECIPE_HOST')
    port = os.environ.get('DBM_POSTGRESQL_RECIPE_PORT')
    dsn = f'host={host} port={port} dbname=dbm user=dbm'
    with psycopg2.connect(dsn) as conn:
        cursor = conn.cursor()
        cursor.execute('SHOW lock_timeout')
        assert cursor.fetchone()[0] == '5s'
