"""
PgShard class to handle several postgres instances in 1 shard
"""
import os
import psycopg2

from .host import PgHost


class PgShard:
    def __init__(self, hosts_config, users, code_dir, code_files):
        self._users = [x for x in (users or ())]
        self._code_dir = code_dir
        self._code_files = [x for x in (code_files or ())]

        master_config, *repl_configs = hosts_config
        self.master = PgHost(master_config)
        self.replicas = [PgHost.replica_for(self.master, cfg) for cfg in repl_configs]

    def start(self):
        self.master.start()
        for r in self.replicas:
            r.start()

        # We need to turn on synchronous replication if shard has replicas
        if self.replicas:
            self.master.run_sql('ALTER SYSTEM SET synchronous_standby_names TO \'*\'')
            self.master.run_sql('SELECT pg_reload_conf()')

        self._init_db()

    @property
    def hosts(self):
        return ','.join((r.host for r in self.all_hosts))

    @property
    def ports(self):
        return ','.join((r.port for r in self.all_hosts))

    @property
    def all_hosts(self):
        return (self.master,) + tuple(self.replicas)

    def _init_db(self):
        with psycopg2.connect(self.master.dsn) as conn:
            with conn.cursor() as cursor:
                for role in self._users:
                    cursor.execute(f'CREATE ROLE {role} LOGIN')
                for code in self._code_files:
                    with open(os.path.join(self._code_dir, code)) as code_file:
                        cursor.execute(code_file.read())
            conn.commit()
