"""Application code"""

import argparse
import datetime
import os
import subprocess
import tempfile
import textwrap
from typing import Dict, List, Tuple
import yaml

from library.python import resource
from library.python.retry import retry_call, RetryConf

from bootstrap.common.console_ui import ask_confirmation
from bootstrap.common.exceptions import BootstrapError
from bootstrap.common.rdbms.db import DbConfig, Db


class EActions:
    CONSOLE = "console"  # invoke pqsl console
    STATUS = "status"  # check and show db status
    ERASE = "erase"  # purge all database content and schema (very dangerous)
    POPULATE = "populate"  # bootstrap database (initialize schema and fill with required data)
    DUMP = "dump"  # dump database with data
    COMPARE_SCHEMA = "compare-schema"  # compare database schema with schema from file (schema is pg-dumped)
    START_DB_IN_DOCKER = "start-db-in-docker"  # start bare postgres database in docker container
    MIGRATE = "migrate"  # migrate database to specified version
    ALL = [CONSOLE, STATUS, ERASE, POPULATE, DUMP, COMPARE_SCHEMA, START_DB_IN_DOCKER, MIGRATE]


def _pg_utils_generic_args(db_config: DbConfig) -> Tuple[List[str], Dict[str, str]]:
    """Generate arguments for various postgres utilities"""
    args = [
        "--host", db_config.host, "--port", str(db_config.port), "--dbname", db_config.dbname,
        "--user", db_config.user,
    ]

    env = {
        "PGPASSWORD": db_config.password,
    }

    return args, env


def _psql_args_env(db_config: DbConfig) -> Tuple[List[str], Dict[str, str]]:
    """Generate arguments <psql> utility"""
    args, env = _pg_utils_generic_args(db_config)
    args = ["/usr/bin/psql"] + args + ["--set", "AUTOCOMMIT=off", "--expanded"]
    return args, env


def _pg_dump_args(db_config: DbConfig) -> Tuple[List[str], Dict[str, str]]:
    """Generate arguments for <pg_dump> utility"""
    args, env = _pg_utils_generic_args(db_config)
    args = ["/usr/bin/pg_dump"] + args + ["--inserts",  "--schema", "public", "--no-privileges", "--no-owner"]
    return args, env


def get_parser():
    parser = argparse.ArgumentParser("Bootstrap db manipulation script")

    subparsers = parser.add_subparsers(dest="action", required=True, help="Action to execute")

    # add arguments for console
    console_parser = subparsers.add_parser(EActions.CONSOLE)
    DbConfig.update_parser(console_parser)

    # add arguments for status
    status_parser = subparsers.add_parser(EActions.STATUS)
    DbConfig.update_parser(status_parser)

    # add arguments for purge database
    erase_parser = subparsers.add_parser(EActions.ERASE)
    erase_parser.add_argument("--devel-database", action="store_true", default=False,
                              help="Flag to identify db as development db (some actions are forbidden on prod db)")
    DbConfig.update_parser(erase_parser)

    # add arguments for populate database
    populate_parser = subparsers.add_parser(EActions.POPULATE)
    populate_parser.add_argument("--devel-database", action="store_true", default=False,
                                 help="Flag to identify db as development db (some actions are forbidden on prod db)")
    DbConfig.update_parser(populate_parser)
    populate_parser.add_argument("--bootstrap-file", type=str, metavar="FILE", required=True,
                                 help="Sql-file to create database scheme")
    populate_parser.add_argument("--version", type=str, default=None,
                                 help="Set database schema version (if not already specified in bootstrap file)")

    # add arguments for dump
    dump_parser = subparsers.add_parser(EActions.DUMP)
    DbConfig.update_parser(dump_parser)
    dump_parser.add_argument("--output-file", type=str, metavar="FILE", required=True,
                             help="File to dump data")
    dump_parser.add_argument("--schema-only", action="store_true", default=False,
                             help="Flag to dump only database schema")

    # add arguments for compare schema
    compare_schema_parser = subparsers.add_parser(EActions.COMPARE_SCHEMA)
    DbConfig.update_parser(compare_schema_parser)
    compare_schema_parser.add_argument("--expected-schema-file", type=str, metavar="FILE", required=True,
                                       help="File to dump data")

    # start postgres db in docker
    start_db_in_docker_parser = subparsers.add_parser(EActions.START_DB_IN_DOCKER)
    start_db_in_docker_parser.add_argument("--populate-current-db", action="store_true", default=False,
                                           help="Populate current database")
    start_db_in_docker_parser.add_argument("--db-version", type=str,
                                           help="Set current db version after populating")

    # migrate database
    migrate_parser = subparsers.add_parser(EActions.MIGRATE)
    DbConfig.update_parser(migrate_parser)
    migrate_parser.add_argument("--migrate-file", type=str, metavar="FILE", required=True,
                                help="Sql-file to mibrate database")
    migrate_parser.add_argument("--version", type=str, required=True,
                                help="Database schema version")

    return parser


def main_console(options: argparse.Namespace) -> int:
    db = Db(DbConfig.from_argparse(options))

    cmd, env = _psql_args_env(db.config)
    env = {**os.environ.copy(), **env}

    os.execve(cmd[0], cmd, env)

    return 0


def main_status(options: argparse.Namespace) -> int:
    db = Db(DbConfig.from_argparse(options), connect=True)

    print("Database <{}>:".format(db.url))
    print("    Bootstrapped: {}".format(db.bootstrapped))
    print("    Version: {}".format(db.version))

    return 0


def _erase(db: Db) -> None:
    # first drop all active connections otherwise our process hangs
    db.drop_active_connections()

    # drop all tables/sequences
    purge_table_query_tpl = textwrap.dedent("""\
        DO $$ DECLARE
            r RECORD;
        BEGIN
            FOR r IN (SELECT {field_name} FROM {table_name} WHERE schemaname = 'public') LOOP
                EXECUTE 'DROP {obj_type} IF EXISTS ' || quote_ident(r.{field_name}) || ' CASCADE';
            END LOOP;
        END $$;
    """)
    cursor = db.conn.cursor()
    for field_name, table_name, obj_type in [
        ("tablename", "pg_tables", "TABLE"),
        ("sequencename", "pg_sequences", "SEQUENCE"),
    ]:
        purge_query = purge_table_query_tpl.format(field_name=field_name, table_name=table_name, obj_type=obj_type)
        cursor.execute(purge_query)
    db.conn.commit()

    # drop all user-defined types
    purge_type_query = textwrap.dedent("""\
        DO $$ DECLARE
            r RECORD;
        BEGIN
            FOR r IN (SELECT t.typname FROM pg_catalog.pg_type t JOIN  pg_catalog.pg_namespace n ON n.oid = t.typnamespace WHERE n.nspname = 'public' AND t.typname NOT LIKE '\_%') LOOP
                EXECUTE 'DROP TYPE IF EXISTS ' || quote_ident(r.typname) || ' CASCADE';
            END LOOP;
        END $$;
    """)
    cursor = db.conn.cursor()
    cursor.execute(purge_type_query)
    db.conn.commit()

    db.update_versions_info()


def main_erase(options: argparse.Namespace) -> int:
    db = Db(DbConfig.from_argparse(options))

    if not options.devel_database:
        raise Exception("Database <{}> is not a development database, erasing is forbidden...".format(db.url))

    if ask_confirmation(question="Purge ALL data from database <{}>".format(db.url)):
        _erase(db)
        print("    Database <{}> was purged".format(db.url))
        return 0
    else:
        print("    Database <{}> was NOT purged".format(db.url))
        return 1


def _populate(db: Db, populate_requests: str, version: str) -> None:
    """Bootstrap database"""
    if db.bootstrapped:
        raise Exception("Database <{}> is already bootstrapped".format(db.url))

    cursor = db.conn.cursor()
    cursor.execute(populate_requests)

    if version:
        cursor.execute("INSERT INTO scheme_info (version) VALUES (%s);", (version,))

    db.conn.commit()

    db.update_versions_info()


def main_populate(options: argparse.Namespace) -> int:
    db = Db(DbConfig.from_argparse(options))

    if not options.devel_database:
        raise Exception("Database <{}> is not a development database, populating is forbidden...".format(db.url))

    if ask_confirmation(question="Bootstrap database <{}>".format(db.url)):
        with open(options.bootstrap_file) as f:
            _populate(db, f.read(), options.version)
        print("    Database <{}> was bootstrapped".format(db.url))
        return 0
    else:
        print("    Database <{}> was NOT bootstrapped".format(db.url))
        return 1


def main_dump(options: argparse.Namespace) -> int:
    db = Db(DbConfig.from_argparse(options))

    cmd, env = _pg_dump_args(db.config)
    cmd.extend(["--file", options.output_file])
    if options.schema_only:
        cmd.append("--schema-only")

    subprocess.check_output(cmd, env=env)

    return 0


def main_compare_schema(options: argparse.Namespace) -> int:
    db = Db(DbConfig.from_argparse(options))

    with tempfile.TemporaryDirectory(prefix="yc-bootstrap-db-admin") as tmp_dir:
        schema_file = os.path.join(tmp_dir, "pg_dump.sql")

        # dump schema
        dump_cmd, dump_env = _pg_dump_args(db.config)
        dump_cmd.extend(["--file", schema_file, "--schema-only"])
        subprocess.check_output(dump_cmd, env=dump_env)

        # compare dumped schema with expected
        diff_cmd = ["/usr/bin/diff", "-u", schema_file, options.expected_schema_file]
        p = subprocess.run(diff_cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, encoding="utf-8")
        if p.returncode == 0:
            print("Database <{}> schema is equal to schema in file <{}>".format(db.url, options.expected_schema_file))
            return 0
        elif p.returncode == 1:
            print("Database <{}> schema differs from expected from file <{}> schema:".format(
                db.url, options.expected_schema_file
            ))
            print(p.stdout)
            return 1
        else:
            raise BootstrapError("Command <{}> exited with status <{}>".format(diff_cmd, p.returncode))


def main_start_db_in_docker(options: argparse.Namespace) -> int:
    """Raise bare db for debug purporses"""
    with tempfile.TemporaryDirectory(prefix="yc-bootstrap-db-admin") as tmp_dir:
        # start postgres in docker
        docker_compose_file = os.path.join(tmp_dir, "docker-compose.yml")
        with open(docker_compose_file, "w") as f:
            f.write(resource.find("docker-compose-local-postgres.yml").decode("utf-8"))
        cmd = ["/usr/bin/docker-compose", "--file", docker_compose_file, "up", "-d"]
        subprocess.check_output(cmd)

        print("Postgres started in docker container (use <docker ps> to find container)")

        # populate if required
        if options.populate_current_db:
            retry_conf = RetryConf(max_time=datetime.timedelta(seconds=60))
            db_config = DbConfig(yaml.load(resource.find("localdb.yaml").decode("utf-8"), Loader=yaml.SafeLoader))

            db = retry_call(Db, (db_config, True), conf=retry_conf)

            _populate(db, resource.find("current.bootstrap.sql"), options.db_version)

            print("Current schema populated to database in docker container")

    return 0


def _migrate(db: Db, migrate_file: str, version: str) -> None:
    """Migrate database

       FIXME: currently we have not checks on migration process:
          - migrate file could be incorrect version
          - current schema could be already modified
          - no checks after migration on database scheme equals to expected
          - ...
    """
    cursor = db.conn.cursor()

    # start migration
    cursor.execute("UPDATE scheme_info SET migrating_to_version = %s", (version, ))
    db.conn.commit()

    # inform all clients about migration started
    db.drop_active_connections()

    # migrate as described in mingrate file
    with open(migrate_file) as f:
        cursor.execute(f.read())
    db.conn.commit()

    # finish migration
    cursor.execute("UPDATE scheme_info SET version = %s, migrating_to_version = NULL", (version,))
    db.conn.commit()

    # inform all clients about migration finished
    db.drop_active_connections()


def main_migrate(options: argparse.Namespace) -> int:
    db = Db(DbConfig.from_argparse(options))

    if ask_confirmation(question="Migrate database <{}> to version <{}>".format(db.url, options.version)):
        _migrate(db, options.migrate_file, options.version)
        print("    Database <{}> was successfully migrated to version <{}>".format(db.url, options.version))
        return 0
    else:
        print("    Database <{}> was NOT migrated".format(db.url))
        return 1


_ACTIONS = {
    EActions.CONSOLE: main_console,
    EActions.STATUS: main_status,
    EActions.ERASE: main_erase,
    EActions.POPULATE: main_populate,
    EActions.DUMP: main_dump,
    EActions.COMPARE_SCHEMA: main_compare_schema,
    EActions.START_DB_IN_DOCKER: main_start_db_in_docker,
    EActions.MIGRATE: main_migrate,
}


def validate_options(options: argparse.Namespace) -> None:
    """Extra validation"""
    if options.action == EActions.START_DB_IN_DOCKER and \
            options.populate_current_db and \
            options.db_version is None:
        raise BootstrapError("You must specify <--db-version> when specifying <--populate-current-db>")


def main(options: argparse.Namespace) -> int:
    return _ACTIONS[options.action](options)
