import os
import psycopg2
from cloud.mdb.recipes.postgresql.lib import env_postgres_addr


def dsn(hostname):
    host, port = env_postgres_addr(hostname)
    return 'host={host} port={port} dbname=postgres'.format(
        host=os.environ.get(host), port=os.environ.get(port))


class Test_replication:

    def test_pg_stat_replication(self):
        with psycopg2.connect(dsn('master')) as conn:
            conn.autocommit = True
            with conn.cursor() as cursor:
                cursor.execute('SELECT count(*) FROM pg_stat_replication')
                count, = cursor.fetchone()
                assert count == 1

    def test_pg_stat_wal_receiver(self):
        with psycopg2.connect(dsn('replica')) as conn:
            conn.autocommit = True
            with conn.cursor() as cursor:
                cursor.execute('SELECT count(*) FROM pg_stat_wal_receiver')
                count, = cursor.fetchone()
                assert count == 1
