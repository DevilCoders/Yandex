# -*- coding: utf-8 -*-
"""
DBaaS Internal API MetaDB pool helpers
"""

import random
import time

import psycopg2
from psycopg2.extras import RealDictCursor
from flask import current_app

from dbaas_common import retry

from ..pool import BasePoolGovernor, GetPoolError
from .node_pool import NodePool, PoolExhaustedError


def pool_filter(pool, master):
    """
    Check if pool is ready to be used
    """
    if not pool['alive']:
        return False
    if master and not pool['master']:
        return False
    # Reserve one connection for alive check
    return pool['pool'].stats.free > 1


class PoolGovernor(BasePoolGovernor):
    """
    PostgreSQL-specific pool-governor
    """

    def __init__(self, config, logger_name):
        BasePoolGovernor.__init__(self, 'metadb-pool-governor', config, logger_name + '.pool_governor')
        for host in self.config['hosts']:
            self.pools[host] = {
                'alive': False,
                'master': False,
                'pool': None,
            }

    def _init_pool(self, host):
        return NodePool(
            self.config['minconn'],
            self.config['maxconn'],
            host=host,
            dbname=self.config['dbname'],
            user=self.config['user'],
            password=self.config['password'],
            port=self.config['port'],
            sslmode=self.config['sslmode'],
            connect_timeout=self.config['connect_timeout'],
            tcp_user_timeout=self.config.get('tcp_user_timeout', 1000),
            keepalives_count=self.config.get('keepalives_count', 3),
            keepalives_interval=self.config.get('keepalives_interval', 1),
            keepalives_idle=self.config.get('keepalives_idle', 1),
            cursor_factory=RealDictCursor,
        )

    @retry.on_exception(GetPoolError, factor=10, max_wait=2, max_tries=5)
    def getpool(self, master=False) -> NodePool:
        """
        Get connection pool
        """
        hosts = [key for key, value in self.pools.items() if pool_filter(value, master)]
        if master and len(hosts) != 1:
            raise GetPoolError('MetaDB is read only')
        if not hosts:
            raise GetPoolError('No alive pool for metadb')

        random.shuffle(hosts)

        return self.pools[self._get_local_host(hosts)]['pool']

    def _report_to_sentry(self):
        sentry_client = getattr(current_app, 'raven_client', None)
        if not sentry_client:
            return
        try:
            sentry_client.captureException()
        except Exception as exc:
            self.logger.warning('report to Sentry failed: %s', exc)

    def run(self):
        self.logger.info('Starting metadb pool governor')
        while self.should_run:
            try:
                self.run_one_cycle()
            except Exception as exc:
                self.logger.exception('pool governor cycle failed unexpectedly: %s', exc)
                self._report_to_sentry()
            time.sleep(1)

    def _recreate_node_pool_if_need_it(self, host: str) -> None:
        pool = self.pools[host]
        if self.pools[host]['alive']:
            try:
                pool['pool'].grow()
            except psycopg2.Error as grow_exc:
                self.logger.warning('failed to grow pool: %s', grow_exc, extra={'metadb_host': host})
            return
        self.logger.info('recreating pool to %r', self.pools[host]['pool'])
        try:
            pool['pool'].recreate()
            pool['pool'].grow()
        except psycopg2.Error as recreate_exc:
            self.logger.warning('failed to recreate dead pool: %s', recreate_exc, extra={'metadb_host': host})

    def _check_node_pool(self, host: str) -> None:
        node_pool = self.pools[host]
        is_alive = False
        try:
            with node_pool['pool'].c_getconn() as conn, conn.cursor() as cur:
                cur.execute('SHOW transaction_read_only')
                node_pool['master'] = bool(cur.fetchone()['transaction_read_only'] == 'off')
                is_alive = True
        except PoolExhaustedError as exc:
            is_alive = node_pool['alive']
            self.logger.warning(
                'pool exhausted: %s, pool recheck skipped at that cycle. Reusing previous alive status: %r',
                exc,
                is_alive,
                extra={'metadb_host': host},
            )
        except psycopg2.Error as exc:
            self.logger.warning('Metadb node is dead: %s', exc, extra={'metadb_host': host})
        except Exception as exc:
            self.logger.exception('unexpected pool check error: %s', exc, extra={'metadb_host': host})
            self._report_to_sentry()

        if node_pool['alive'] != is_alive:
            self.logger.info(
                'Metadb alive' if is_alive else 'Metadb dead',
                extra={
                    'metadb_host': host,
                },
            )
        node_pool['alive'] = is_alive

    def run_one_cycle(self) -> None:
        for host in self.config['hosts']:
            if self.pools[host]['pool'] is None:
                try:
                    self.pools[host]['pool'] = self._init_pool(host)
                except psycopg2.Error as exc:
                    self.logger.warning('failed to init pool: %s', exc, extra={'metadb_host': host})
                    # we don't have a pool, so can't check it
                    # better go to the next host
                    continue
            self._check_node_pool(host)
            self._recreate_node_pool_if_need_it(host)
