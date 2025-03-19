"""
PgHost clss to handle 1 postgres instance
"""
from copy import deepcopy
import psycopg2

from cloud.mdb.recipes.postgresql.lib import start as generic_start


class PgHost:
    def __init__(self, config):
        self._name = config['name']
        self._db = config['db']
        self._dc = config['dc']
        self._db_config = deepcopy(config['config'])

        self._source_path = config.get('source_path')
        self._grants_path = config.get('grants_path')
        self._before_migration_sql = config.get('before_migration_sql')

        self._repl_master = None
        self._host = None
        self._port = None

    @classmethod
    def replica_for(cls, master, config):
        host = cls(config)
        host._repl_master = master
        return host

    def start(self):
        if self._host is not None:
            raise Exception("already started")
        self._host, self._port = generic_start(self._config)

    def run_sql(self, sql):
        with psycopg2.connect(self.dsn) as conn:
            conn.autocommit = True
            with conn.cursor() as cursor:
                cursor.execute(sql)

    @property
    def name(self):
        return self._name

    @property
    def dc(self):
        return self._dc

    @property
    def db(self):
        return self._db

    @property
    def is_master(self):
        return not bool(self._repl_master)

    @property
    def host(self):
        return self._host

    @property
    def port(self):
        return self._port

    @property
    def dsn(self):
        if self._host is None:
            return None
        return f'host={self._host} port={self._port} dbname={self._db}'

    @property
    def host_repl(self):
        if self._host is None:
            return None
        return f'host={self._host} port={self._port}'

    @property
    def _repl_source(self):
        if not self._repl_master:
            return None
        return self._repl_master.host_repl

    @property
    def _config(self):
        cfg = {
            'name': self._name,
            'replication_source': self._repl_source,
            'config': self._db_config,
            'db': self._db or 'postgres',
            'postgres_custom_variable': {'pgcheck.closed': 'false'},
        }
        if self._before_migration_sql:
            cfg['before_migration_sql'] = self._before_migration_sql
        if self._source_path:
            cfg['source_path'] = self._source_path
        if self._grants_path:
            cfg['grants_path'] = self._grants_path
        return cfg
