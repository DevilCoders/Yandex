import os

import psycopg2


class Test_connection:
    def test_with_simple_select(self):
        host = os.environ.get('SALT_POSTGRESQL_RECIPE_HOST')
        port = os.environ.get('SALT_POSTGRESQL_RECIPE_PORT')
        dsn = f'host={host} port={port} dbname=db1'
        with psycopg2.connect(dsn) as conn:
            cursor = conn.cursor()
            cursor.execute('SELECT 1')
