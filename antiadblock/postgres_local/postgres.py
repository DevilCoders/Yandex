"""
Class for using local postgresql in Arcadia tests. it wraps calls to postgres binaries retrieved from Sandbox. If you need custom migrations you should implement your own Migrator class
"""

import logging
import os

import shutil
import signal
import uuid

import yatest.common

from antiadblock.postgres_local.exceptions import PostgresqlException
from antiadblock.postgres_local.migrator import Migrator
from sqlalchemy import create_engine

DATABASE_INIT_TIMEOUT_SEC = int(os.environ.get('PG_LOCAL_DATABASE_INIT_TIMEOUT_SEC', 20))


class Config(object):
    def __init__(
            self,
            port=5432, keep_work_dir=False, work_dir=None,
            username='postgres', password='postgres', dbname='postgres',
            disable_fsync=False, wal_level=None, include_wal2json=False
    ):
        self.port = port
        self.keep_work_dir = keep_work_dir
        self.work_dir = work_dir
        self.username = username
        self.password = password
        self.dbname = dbname
        self.disable_fsync = disable_fsync
        self.wal_level = wal_level
        self.include_wal2json = include_wal2json


class State(object):
    NONE, INITIALIZING, MIGRATING, RUNNING = "NONE, INITIALIZING, MIGRATING, RUNNING".split(', ')


class PostgreSQL(object):
    def __init__(self, config=None, migrator=Migrator()):
        self.logger = logging.getLogger("postgres-local")
        self.migrator = migrator
        self.config = config or Config()
        self.instance_id = str(uuid.uuid4())
        self.work_dir = (
            self.config.work_dir or yatest.common.output_path(self.instance_id)
        )
        self.state = State.NONE
        self.engine = None

        self.db_path = os.path.join(yatest.common.work_path(), 'pgsql')

        if not os.path.exists(self.db_path):
            raise PostgresqlException("No postgresql binaries found in {}".format(self.db_path))

        self.data_path = os.path.join(self.work_dir, 'data')
        self.bin_path = os.path.join(self.db_path, 'bin')

        self.pg_ctl_local_path = os.path.join(self.bin_path, 'pg_ctl')
        self.postgres_pid_file = os.path.join(self.data_path, 'postmaster.pid')

        self.user_creator = os.path.join(self.bin_path, 'createuser')
        self.db_creator = os.path.join(self.bin_path, 'createdb')

    def _prepare_files(self):
        self.ensure_state(State.INITIALIZING)

        self.logger.debug('Preparing working directories')

        if os.path.exists(self.work_dir):
            shutil.rmtree(self.work_dir)
        os.mkdir(self.work_dir)

        if os.path.exists(self.data_path):
            shutil.rmtree(self.data_path)
        os.mkdir(self.data_path)

    def _init_database(self):
        # pg_ctl initdb creates postgres database by default
        if self.config.dbname == 'postgres':
            return
        self.ensure_state(State.INITIALIZING)
        cmd = [
            self.db_creator,
            '--host={}'.format('localhost'),
            '--port={}'.format(self.config.port),
            '--lc-collate=ru_RU.UTF-8',
            '--lc-ctype=ru_RU.UTF-8',
            '--template=template0',
            '--username={}'.format(self.config.username),
            '--no-password',
            self.config.dbname,
        ]
        self.logger.debug('Database creation: {}'.format(' '.join(cmd)))
        try:
            create_database = yatest.common.process.execute(cmd, timeout=DATABASE_INIT_TIMEOUT_SEC)
        except Exception:
            self.logger.exception('Errors while database creation', exc_info=True)
            raise
        self.logger.info("Database {} initialized: {}".format(self.config.dbname, create_database.std_out))

    def _init_database_cluster(self):
        self.ensure_state(State.INITIALIZING)
        cmd = [
            self.pg_ctl_local_path,
            'initdb',
            '-o', '--auth-local=trust',
            '-o', '--encoding=UTF8',
            '-o', '--username={}'.format(self.config.username),
            '-D', self.data_path,
        ]
        self.logger.debug('Init data directory: {}'.format(' '.join(cmd)))
        try:
            init_cluster = yatest.common.process.execute(cmd, timeout=DATABASE_INIT_TIMEOUT_SEC)
        except Exception:
            self.logger.exception('Errors while database init', exc_info=True)
            raise

        if self.config.wal_level:
            path_to_config = os.path.join(self.data_path, "postgresql.conf")
            with open(path_to_config, "a") as myfile:
                myfile.write("\nwal_level = {}\n".format(self.config.wal_level))

        self.logger.info("Database cluster initialized: {}".format(init_cluster.std_out))

    def _start(self):
        self.logger.debug('Try to start local psql with id={}'.format(self.instance_id))
        cmd = [
            self.pg_ctl_local_path,
            'start',
            '-o', '--port={}'.format(self.config.port),
            # Disable creation of any UNIX sockets to handle client's requests
            '-o', '-c unix_socket_directories=""',
            '-D', self.data_path,
        ]
        if self.config.disable_fsync:
            cmd.extend(['-o', '--fsync=off'])
        self.logger.debug(' '.join(cmd))
        try:
            yatest.common.process.execute(cmd)
        except Exception:
            self.logger.exception('Failed to start local psql', exc_info=True)
            self._kill()
            raise

    def _kill(self):
        self.ensure_state({State.RUNNING, State.MIGRATING})
        for pid in self._get_pids():
            try:
                os.kill(pid, signal.SIGKILL)
            except OSError:
                pass

    def _get_pids(self):
        if not os.path.exists(self.postgres_pid_file):
            return []
        with open(self.postgres_pid_file, 'r') as pids:
            pid = pids.readline()
            return [int(pid)]

    def _stop_psql(self):
        self.ensure_state({State.RUNNING, State.MIGRATING})
        self.logger.debug('Stop db instance')
        cmd = [
            self.pg_ctl_local_path,
            'stop',
            '-D', self.data_path,
        ]
        self.logger.debug(' '.join(cmd))
        try:
            yatest.common.process.execute(cmd)
        except Exception as e:
            self.logger.debug('Errors while stopping local posgresql: {}'.format(str(e)), exc_info=True)
            raise

    def _migrate(self):
        self.ensure_state(State.MIGRATING)
        self.exec_transaction(lambda connection: self.migrator.migrate(connection))

    def get_sqlalchemy_engine(self):
        self.ensure_state({State.RUNNING, State.MIGRATING})
        if self.engine is None:
            self.engine = create_engine(self._get_sqlalchemy_connection_url(), echo=True)
        return self.engine

    def _get_sqlalchemy_connection_url(self):
        return 'postgresql+psycopg2://{user}:{password}@{host}:{port}/{dbname}'.format(user=self.config.username,
                                                                                       password=self.config.password,
                                                                                       host='localhost',
                                                                                       port=self.config.port,
                                                                                       dbname=self.config.dbname)

    def exec_transaction(self, operation):
        """
        Executes simple sql operation that not require return value
        :param operation:
        :return:
        """
        self.ensure_state({State.RUNNING, State.MIGRATING})

        with self.get_sqlalchemy_engine().connect() as conn:
            tr = None
            try:
                tr = conn.begin()
                operation(conn)
                tr.commit()
            except:
                if tr is not None:
                    tr.rollback()
                raise

    def shutdown(self):
        if self.state in [State.RUNNING, State.MIGRATING]:
            self._stop_psql()
            self._kill()

        if self.state in [State.RUNNING, State.MIGRATING, State.INITIALIZING]:
            if not self.config.keep_work_dir:
                shutil.rmtree(self.work_dir, ignore_errors=True)

            if self.engine is not None:
                self.engine.dispose()

        self.state = State.NONE

    def run(self):
        self.ensure_state(State.NONE)

        self.state = State.INITIALIZING
        self._prepare_files()
        self._init_database_cluster()
        self._start()
        self._init_database()

        self.state = State.MIGRATING
        self._migrate()

        self.state = State.RUNNING

    def ensure_state(self, states):
        if isinstance(states, str):
            states = [states]
        if self.state not in states:
            raise PostgresqlException("State check failed. Expected states: {}. Current state: {}".format(', '.join(states), self.state))
