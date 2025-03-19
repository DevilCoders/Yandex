from contextlib import closing, contextmanager
import os
import logging

import psycopg2
import psycopg2.extensions
import pytest

from dbaas_internal_api.dbs.postgresql.pool import PoolGovernor, GetPoolError
from dbaas_internal_api.dbs.postgresql.node_pool import PoolStats

log = logging.getLogger(__name__)


def meta_host():
    return os.environ['METADB_POSTGRESQL_RECIPE_HOST']


def meta_port():
    return os.environ['METADB_POSTGRESQL_RECIPE_PORT']


def make_admin_conn():
    return psycopg2.connect(f'host={meta_host()} port={meta_port()} dbname=dbaas_metadb')


class TestPoolGovernor:
    user = 'pool_test_user'
    database = 'pool_test_database'
    maxconn = 3

    def recreate_test_env(self):
        with make_admin_conn() as conn:
            conn.autocommit = True
            cur = conn.cursor()
            cur.execute('SELECT count(*) FROM pg_roles WHERE rolname = %s', (self.user,))
            if cur.fetchone()[0]:
                cur.execute(f'DROP ROLE {self.user}')
            cur.execute(f'CREATE ROLE {self.user} WITH NOSUPERUSER LOGIN CONNECTION LIMIT 3')
            cur.execute('SELECT count(*) FROM pg_database WHERE datname = %s', [self.database])
            if not cur.fetchone()[0]:
                cur.execute(f'CREATE DATABASE {self.database}')

    def destroy_test_env(self):
        # Sadly, but dropping user or setting NOLOGIN to a user doesn't breaks pool.
        # Probably it's cause we use `trust` in pg_hba.conf
        with closing(make_admin_conn()) as conn:
            conn.autocommit = True
            cur = conn.cursor()
            cur.execute('SELECT pg_terminate_backend(pid) FROM pg_stat_activity WHERE datname=%s', [self.database])
            log.info('%d sessions terminated', len([r for r in cur.fetchall()]))
            try:
                cur.execute(f'DROP DATABASE {self.database}')
            except psycopg2.Error:
                pass

    @contextmanager
    def new_gv(self) -> PoolGovernor:
        pool_governor = PoolGovernor(
            config={
                'hosts': [meta_host()],
                'port': meta_port(),
                'minconn': 2,
                'maxconn': self.maxconn,
                'dbname': self.database,
                'user': self.user,
                'password': '',
                'sslmode': 'disable',
                'connect_timeout': 1,
            },
            logger_name='pool_governor',
        )
        try:
            yield pool_governor
        finally:
            for host_pool in pool_governor.pools.values():
                if pool := host_pool['pool']:
                    pool.closeall()

    def test_alive(self):
        self.recreate_test_env()
        with self.new_gv() as pool_governor:
            pool_governor.run_one_cycle()
            assert pool_governor.pools[meta_host()]['alive']

    def test_recreate(self):
        self.recreate_test_env()
        with self.new_gv() as pool_governor:
            # On first iteration we create connections
            pool_governor.run_one_cycle()
            acquired_conn = pool_governor.getpool(True).getconn()

            self.destroy_test_env()
            pool_governor.run_one_cycle()
            pool_governor.run_one_cycle()

            assert not pool_governor.pools[meta_host()]['alive']
            # that connection may not works (upper we drop database to which it's connected),
            # but it shouldn't be closed.
            assert not acquired_conn.closed, "acquired connection shouldn't be closed manually"

            self.recreate_test_env()
            pool_governor.run_one_cycle()
            assert pool_governor.pools[meta_host()]['alive'], "pool should be healthy after recreate"

            pool_governor.getpool(True).putconn(acquired_conn)

    def test_recreate_twice(self):
        self.recreate_test_env()
        with self.new_gv() as pool_governor:
            # On first iteration we create connections
            pool_governor.run_one_cycle()
            assert pool_governor.getpool(True).stats == PoolStats(used=0, free=3, open=2)

            for attempt_no in range(2):
                log.info('attempt %r', attempt_no + 1)
                self.destroy_test_env()
                pool_governor.run_one_cycle()
                self.recreate_test_env()
                pool_governor.run_one_cycle()
                assert pool_governor.getpool(True).stats == PoolStats(used=0, free=3, open=2)

    def test_db_available_after_we_create_the_pool(self):
        self.destroy_test_env()
        with self.new_gv() as pool_governor:
            pool_governor.run_one_cycle()
            with pytest.raises(GetPoolError):
                pool_governor.getpool(True)
            self.recreate_test_env()
            pool_governor.run_one_cycle()
            assert pool_governor.pools[meta_host()]['alive']

    def test_exhausted_pool_check(self):
        self.recreate_test_env()
        with self.new_gv() as pool_governor:
            pool_governor.run_one_cycle()
            pool = pool_governor.getpool(True)
            for _ in range(self.maxconn):
                pool.getconn()
            pool_governor.run_one_cycle()
            assert pool_governor.pools[meta_host()]['alive']
