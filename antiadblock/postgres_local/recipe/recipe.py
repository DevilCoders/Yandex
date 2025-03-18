import argparse
import json
import logging
import os
import shutil
import signal

from antiadblock.postgres_local.migrator import Migrator, FileBasedMigrator
from antiadblock.postgres_local.postgres import Config, PostgreSQL, State
from library.python.testing.recipe import declare_recipe, set_env
from yatest.common.network import PortManager
from yatest.common.process import execute
from yatest.common.runtime import source_path, work_path


class PostgresRecipe():
    def __init__(self):
        self.recipe_info_json_file = "pg_local_recipe_info.json"

        self.logger = logging.getLogger('postgres-local')
        self.logger.setLevel(logging.INFO)
        self.logger.addHandler(logging.StreamHandler())

    def parse_args(self, args):
        parser = argparse.ArgumentParser()
        parser.add_argument('--port', type=int, dest='port', nargs='?', default=5432,
                            help='port that is used with --no-random-ports option')
        parser.add_argument('--user', type=str, dest='user', nargs='?', default='postgres',
                            help='user to create')
        parser.add_argument('--password', type=str, dest='password', nargs='?', default='postgres',
                            help='password for user')
        parser.add_argument('--db_name', type=str, dest='db_name', nargs='?', default='postgres',
                            help='database to create')
        parser.add_argument('--migrations_path', type=str, dest='migrations_path',
                            help='migrations folder to apply to database. If no migrations_path set then no migrations will be apply')
        parser.add_argument('--use-text', action='store_true', help='Wrap migrations using sqlalchemy.text')
        parser.add_argument('--disable-fsync', action='store_true', help='disable fsync')
        parser.add_argument('--env_prefix', type=str, dest='env_prefix', default='',
                            help='set env_prefix to host several DB\'s alongside each other so that their environmental variables won\'t clash')
        parser.add_argument('--wal_level', type=str, dest='wal_level',
                            help='wal_level: minimal, replica, or logical. default: replica')
        parser.add_argument('--include-wal2json', action='store_true', dest='include_wal2json', help='include plugin wal2json')
        args, _ = parser.parse_known_args(args=args)
        return args

    def start(self, args):
        self.logger.info("Starting local postgresql. Args: {}".format(args))
        args = self.parse_args(args)

        migrator = Migrator()  # no migrations will be applied
        if args.migrations_path is not None:
            migrator = FileBasedMigrator(source_path(args.migrations_path), args.use_text)

        psql_config = Config(port=PortManager().get_port(args.port),
                             keep_work_dir=False,
                             username=args.user,
                             password=args.password,
                             dbname=args.db_name,
                             disable_fsync=args.disable_fsync,
                             wal_level=args.wal_level,
                             include_wal2json=args.include_wal2json)

        postgresql = PostgreSQL(psql_config, migrator)

        if args.include_wal2json:
            shutil.copyfile("./wal2json.so", os.path.join(postgresql.db_path, "lib", "postgresql", "wal2json.so"))

        recipe_info = dict()
        recipe_info[args.env_prefix + "PG_LOCAL_USER"] = psql_config.username
        recipe_info[args.env_prefix + "PG_LOCAL_PASSWORD"] = psql_config.password
        recipe_info[args.env_prefix + "PG_LOCAL_PORT"] = psql_config.port
        recipe_info[args.env_prefix + "PG_LOCAL_DATABASE"] = psql_config.dbname
        recipe_info[args.env_prefix + "PG_LOCAL_WORK_DIR"] = postgresql.work_dir
        recipe_info[args.env_prefix + "PG_LOCAL_BIN_PATH"] = postgresql.bin_path

        try:
            postgresql.run()
            postgresql.ensure_state(State.RUNNING)

            self.logger.info("Local postgres recipe config: {}".format(json.dumps(recipe_info)))

            for k, v in recipe_info.items():
                set_env(k, str(v))
        except:
            self.logger.exception("Postgresql start failed")
            raise
        finally:
            # to clean with stop function in any case
            with open(args.env_prefix + self.recipe_info_json_file, "w") as f:
                json.dump(recipe_info, f)

    def stop(self, args):
        self.logger.info("Stopping local postgresql. Args: {}".format(args))
        args = self.parse_args(args)

        def _get_pids(pidfile):
            if not os.path.exists(pidfile):
                return []
            with open(pidfile, 'r') as pids:
                pid = pids.readline()
                return [int(pid)]

        with open(args.env_prefix + self.recipe_info_json_file) as f:
            recipe_info = json.load(f)

        def _stop_postgres(pg_ctl, data_path):
            logging.debug('Stop db instance')
            cmd = [
                pg_ctl,
                'stop',
                '-D', data_path,
            ]
            logging.debug(' '.join(cmd))
            try:
                execute(cmd)
            except Exception as e:
                logging.debug('Errors while stopping local posgresql:\n%s', str(e), exc_info=True)
                pass  # trying to finish stop action

        def _kill_postgres(pidfile):
            for pid in _get_pids(pidfile):
                try:
                    os.kill(pid, signal.SIGKILL)
                except OSError:
                    pass

        def _rm_data_dir(data_path):
            shutil.rmtree(data_path, ignore_errors=True)

        pg_ctl = os.path.join(work_path(), 'pgsql/bin/pg_ctl')
        data_path = os.path.join(recipe_info[args.env_prefix + 'PG_LOCAL_WORK_DIR'], 'data')
        postgres_pid_file = os.path.join(data_path, 'postgres_pid_file')

        _stop_postgres(pg_ctl, data_path)
        _kill_postgres(postgres_pid_file)
        _rm_data_dir(recipe_info[args.env_prefix + 'PG_LOCAL_WORK_DIR'])

        PortManager().release_port(int(recipe_info[args.env_prefix + 'PG_LOCAL_PORT']))


def main():
    recipe = PostgresRecipe()
    declare_recipe(recipe.start, recipe.stop)
