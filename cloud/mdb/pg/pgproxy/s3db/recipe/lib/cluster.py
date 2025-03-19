"""
PgCluster class to handle whole s3 cluster with pgmeta, s3meta, s3db and s3proxy
"""
import os
import psycopg2
import yatest.common

from copy import deepcopy
from functools import partial

from library.python.testing.recipe import set_env

from .shard import PgShard

PG_META_USERS = [
    'monitor',
    'pgproxy',
    's3ro',
]

PG_PROXY_USERS = [
    'monitor',
    's3api',
    's3api_ro',
    's3api_list',
    's3cleanup',
]

COMMON_CODE = [
    'dynamic_query.sql',
]

PG_META_CODE = [
    's3db.sql',
]

PG_PROXY_CODE = [
    'pgproxy.sql',
    'get_cluster_config.sql',
    'get_cluster_version.sql',
    'inc_cluster_version.sql',
    'is_master.sql',
    'dynamic_query.sql',
    's3db/plproxy.sql',
    's3db/update_remote_tables.sql',
    'get_partitions.sql',
    'select_part.sql',
    's3db/v1/code.sql',
    's3db/v1/impl.sql',
    's3db/tests.sql',
]


class PgCluster:
    def __init__(self, config, code_root):
        self._pg_meta_config = deepcopy(config['pgmeta'])
        self._s3_meta_config = deepcopy(config['s3meta'])
        self._s3_db_config = deepcopy(config['s3db'])
        self._pgproxy_config = deepcopy(config['pgproxy'])

        self._common_code_root = yatest.common.source_path(os.path.join(code_root, 'common'))
        self._pg_meta_code_root = yatest.common.source_path(os.path.join(code_root, 'pgmeta'))
        self._pgproxy_code_root = yatest.common.source_path(os.path.join(code_root, 'pgproxy'))

        patch_db_config(self._pg_meta_config, patch_pgmeta_config)
        patch_db_config(self._s3_meta_config, partial(patch_s3meta_config, code_root))
        patch_db_config(self._s3_db_config, partial(patch_s3db_config, code_root))
        patch_db_config(self._pgproxy_config, patch_pgproxy)

        self._pg_meta_shards = None
        self._s3_meta_shards = None
        self._s3_db_shards = None
        self._pgproxy_shards = None

    def start(self):
        self._start_pgmeta()
        self._start_s3meta()
        self._start_s3db()
        self._fill_meta()
        self._update_s3meta_parts()

        self._start_pgproxy()
        self._pgproxy_update_remote_tables()

    def _start_pgmeta(self):
        self._pg_meta_shards = [
            PgShard(s, PG_META_USERS, self._pg_meta_code_root, PG_META_CODE) for s in self._pg_meta_config
        ]

        for s in self._pg_meta_shards:
            s.start()

    def _start_s3meta(self):
        pgmeta_hosts = self._pg_meta_shards[0].hosts
        pgmeta_ports = self._pg_meta_shards[0].ports

        patch_db_config(self._s3_meta_config, partial(patch_s3meta_sql, pgmeta_hosts, pgmeta_ports))

        self._s3_meta_shards = [PgShard(s, (), self._common_code_root, COMMON_CODE) for s in self._s3_meta_config]

        for s in self._s3_meta_shards:
            s.start()

    def _start_s3db(self):
        self._s3_db_shards = [PgShard(s, (), self._common_code_root, COMMON_CODE) for s in self._s3_db_config]

        for s in self._s3_db_shards:
            s.start()

    def _start_pgproxy(self):
        pgmeta_hosts = self._pg_meta_shards[0].hosts
        pgmeta_ports = self._pg_meta_shards[0].ports

        patch_db_config(self._pgproxy_config, partial(patch_pgproxy_sql, pgmeta_hosts, pgmeta_ports))

        self._pgproxy_shards = [
            PgShard(s, PG_PROXY_USERS, self._pgproxy_code_root, PG_PROXY_CODE) for s in self._pgproxy_config
        ]

        for s in self._pgproxy_shards:
            s.start()

    def _pgproxy_update_remote_tables(self):
        for s in self._pgproxy_shards:
            s.master.run_sql('SELECT plproxy.update_remote_tables()')

    def _update_s3meta_parts(self):
        for s in self.s3meta:
            s.master.run_sql('SELECT s3.update_parts()')
            s.master.run_sql('REFRESH MATERIALIZED VIEW s3.shard_stat')

    def _fill_meta(self):
        with psycopg2.connect(self.pgmeta.master.dsn) as conn:
            self._fill_cluster(conn, 1, 'meta', self.s3meta)
            self._fill_cluster(conn, 2, 'db', self.s3db)
            conn.commit()

    def _fill_cluster(self, conn, cluster_id, cluster_name, shards):
        with conn.cursor() as cur:
            cur.execute(
                'INSERT INTO clusters VALUES (%(cluster_id)s, %(name)s)',
                {'cluster_id': cluster_id, 'name': cluster_name},
            )
            # Find current host_id
            cur.execute('SELECT coalesce(max(host_id), 0) FROM hosts')
            (host_id,) = cur.fetchone()
            # Find current part_id
            cur.execute('SELECT coalesce(max(part_id), 0) FROM parts')
            (part_id,) = cur.fetchone()
            for shard in shards:
                part_id += 1
                cur.execute(
                    'INSERT INTO parts VALUES (%(part_id)s, %(cluster_id)s)',
                    {'part_id': part_id, 'cluster_id': cluster_id},
                )
                for host in shard.all_hosts:
                    host_id += 1
                    priority = 0 if host.is_master else 10
                    cur.execute(
                        'INSERT INTO hosts (host_id, host_name, dc, base_prio) VALUES (%(host_id)s, %(name)s, %(dc)s, %(priority)s)',
                        {'host_id': host_id, 'name': host.name, 'dc': host.dc, 'priority': priority},
                    )
                    cur.execute(
                        'INSERT INTO connections VALUES (%(conn_id)s, %(connstring)s)',
                        {
                            'conn_id': host_id,
                            'connstring': 'host={host} port={port} dbname={dbname}'.format(
                                host=host.host, port=host.port, dbname=host.db
                            ),
                        },
                    )
                    cur.execute(
                        'INSERT INTO priorities VALUES (%(part_id)s, %(host_id)s, %(conn_id)s, %(priority)s)',
                        {'part_id': part_id, 'host_id': host_id, 'conn_id': host_id, 'priority': priority},
                    )

    def set_env(self):
        pgmeta = self.pgmeta
        for h in pgmeta.all_hosts:
            set_env(h.name.upper(), h.dsn)

        for s in self.s3meta:
            for h in s.all_hosts:
                set_env(h.name.upper(), h.dsn)

        for s in self.s3db:
            for h in s.all_hosts:
                set_env(h.name.upper(), h.dsn)

        pgproxy = self.pgproxy
        for h in pgproxy.all_hosts:
            set_env(h.name.upper(), h.dsn)
        set_env('PROXY_HOST', pgproxy.master.host)
        set_env('PROXY_PORT', pgproxy.master.port)
        set_env('PROXY_DBNAME', pgproxy.master.db)

    @property
    def pgmeta(self):
        return self._pg_meta_shards[0]

    @property
    def s3meta(self):
        return self._s3_meta_shards

    @property
    def s3db(self):
        return self._s3_db_shards

    @property
    def pgproxy(self):
        return self._pgproxy_shards[0]


def _patch_config(cfg):
    cfg.update(
        {
            'config': {
                'max_connections': 64,
                'max_prepared_transactions': 10,
                'shared_preload_libraries': 'repl_mon',
                'synchronous_commit': 'remote_apply',
            },
        }
    )


def patch_pgmeta_config(cfg):
    _patch_config(cfg)
    cfg.update(
        {
            'db': 's3db',
        }
    )


def patch_s3meta_config(code_root, cfg):
    _patch_config(cfg)
    cfg.update(
        {
            'db': 's3meta',
            'source_path': os.path.join(code_root, 's3meta'),
            'grants_path': 'v1/grants',
        }
    )


def patch_s3meta_sql(pgmeta_hosts, pgmeta_ports, cfg):
    cfg[
        'before_migration_sql'
    ] = f"""
        CREATE EXTENSION postgres_fdw CASCADE;
        CREATE SERVER pgmeta FOREIGN DATA WRAPPER postgres_fdw
            OPTIONS (host '{pgmeta_hosts}', port '{pgmeta_ports}', dbname 's3db', updatable 'false');
        CREATE USER MAPPING FOR CURRENT_USER SERVER pgmeta OPTIONS (user 'pgproxy');
        IMPORT FOREIGN SCHEMA public LIMIT TO (clusters, parts) FROM SERVER pgmeta INTO public;
    """


def patch_s3db_config(code_root, cfg):
    _patch_config(cfg)
    cfg.update(
        {
            'db': 's3db',
            'source_path': os.path.join(code_root, 's3db'),
            'grants_path': 'v1/grants',
        }
    )
    cfg['config'].update(
        {
            'shared_preload_libraries': 'repl_mon',
        }
    )


def patch_pgproxy(cfg):
    _patch_config(cfg)
    cfg.update(
        {
            'db': 's3db',
        }
    )


def patch_pgproxy_sql(pgmeta_hosts, pgmeta_ports, cfg):
    cfg[
        'before_migration_sql'
    ] = f"""
        CREATE EXTENSION postgres_fdw CASCADE;
        CREATE SERVER remote FOREIGN DATA WRAPPER postgres_fdw
            OPTIONS (host '{pgmeta_hosts}', port '{pgmeta_ports}', dbname 's3db', updatable 'false');
        CREATE USER MAPPING FOR CURRENT_USER SERVER remote OPTIONS (user 'pgproxy');
    """


def patch_db_config(cfg, patch_func):
    for shard in cfg:
        for host in shard:
            patch_func(host)
