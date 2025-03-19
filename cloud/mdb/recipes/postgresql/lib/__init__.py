"""
Recipe for postgres
"""
from enum import Enum, unique
import errno
import logging
import os
import os.path
import shutil
import subprocess
import sys
import tempfile
import time
from typing import Dict, Any

import psycopg2
import psycopg2.errors
from pgmigrate import get_config, migrate

import yatest.common

from cloud.mdb.recipes.postgresql.lib.persistence import PersistenceConfig, ClusterState
from library.python.testing.recipe import set_env


class MigrationError(Exception):
    pass


class InvalidMigrationError(MigrationError):
    pass


class DuplicateMigrationError(MigrationError):
    pass


class LostMigrationError(MigrationError):
    pass


class FakePgMigrateArgs:
    """
    Fake args for pgmigrate
    """

    # pylint: disable=too-few-public-methods

    def __init__(self, dsn):
        self.conn = dsn


def _wait_postgres_ready(host, port, timeout=10):
    dsn = f'host={host} port={port} dbname=postgres'
    for _ in range(int(timeout)):
        try:
            with psycopg2.connect(dsn):
                break
        except psycopg2.errors.OperationalError:
            time.sleep(1)


def run_sql(socket_dir, postgres_port, sql, dbname='postgres'):
    dsn = f'host={socket_dir} port={postgres_port} dbname={dbname}'
    with psycopg2.connect(dsn) as conn:
        conn.autocommit = True
        with conn.cursor() as cursor:
            cursor.execute(sql)


def _check_for_lost_migrations(migrations):
    """
    Check for gaps in migration versions (the same way as pgmigrate does it)
    """
    sorted_versions = sorted(migrations)
    first = sorted_versions[0]
    last = sorted_versions[-1]
    if last - first + 1 != len(sorted_versions):
        versions = set(sorted_versions)
        missing = [str(x) for x in range(first, last) if x not in versions]
        raise LostMigrationError(f'You have lost migrations: {", ".join(missing)}')


def _validate_migrations(base_path):
    migration_path = os.path.join(base_path, 'migrations')
    prefix = 'V'
    separator = '__'
    migrations = []
    for migration in os.listdir(migration_path):
        version, _, name = migration.partition(separator)
        if not name or not version.startswith(prefix):
            raise InvalidMigrationError(f'Invalid migration file name "{migration}"')
        if version in migrations:
            raise DuplicateMigrationError(
                f'Duplicate {version} migrations detected: "{migration}" and "{migrations[version]}"'
            )
        migrations.append(int(version.lstrip(prefix)))

    _check_for_lost_migrations(migrations)


def _run_migrations(socket_dir, postgres_port, config, dbname):
    source_path = config['source_path']

    dsn = f'host={socket_dir} port={postgres_port} dbname={dbname}'

    base_path = yatest.common.source_path(source_path)

    _validate_migrations(base_path)

    users = config.get('users', [])
    if len(users) == 0:
        grants_path = config.get('grants_path', 'grants')
        for grant_file in os.listdir(os.path.join(base_path, grants_path)):
            users.append(grant_file.split('.')[0])

    with psycopg2.connect(dsn) as conn:
        cursor = conn.cursor()
        for user in users:
            cursor.execute(f"CREATE USER {user} WITH PASSWORD '{user}pw'")

    # We do chdir trick here because all paths in migrations.yml are not absolute
    path = os.getcwd()
    os.chdir(base_path)
    migrate(get_config('.', FakePgMigrateArgs(dsn)))
    os.chdir(path)


def _pid_dir():
    return 'pids/'


def _pid_file(name):
    return f'postgres-{name}.pid'


def _pid_path(name):
    return os.path.join(_pid_dir(), _pid_file(name))


def env_postgres_addr(name):
    env_prefix = name.upper()
    return f'{env_prefix}_POSTGRESQL_RECIPE_HOST', f'{env_prefix}_POSTGRESQL_RECIPE_PORT'


def pg_resource_dir():
    return os.path.join(yatest.common.work_path(), 'pg')


@unique
class Mode(Enum):
    performance = 'PERFORMANCE'
    stable = 'STABLE'


def start(config: Dict[str, Any], mode=Mode.performance):
    cluster_config = config
    persistence_config = PersistenceConfig.read()

    if persistence_config and cluster_config['name'] in persistence_config.persist_clusters:
        cluster = start_persistent_(persistence_config, cluster_config, mode=mode)
    else:
        cluster = start_local_(cluster_config, mode=mode)

    env_host, env_port = env_postgres_addr(cluster.name)
    set_env(env_host, cluster.socket_dir)
    set_env(env_port, str(cluster.port))
    return cluster.socket_dir, str(cluster.port)


def start_local_(config, mode=Mode.performance) -> ClusterState:
    cluster = start_cluster_(
        config=config,
        mode=mode,
        pg_dir=pg_resource_dir(),
        work_dir=os.path.join(
            yatest.common.ram_drive_path() or pg_resource_dir(), 'wd_{name}'.format(name=config['name'])
        ),
        socket_dir=tempfile.mkdtemp(prefix='socket_'),
    )

    os.makedirs(_pid_dir(), exist_ok=True)
    with open(_pid_path(config['name']), 'w') as out:
        out.write(str(cluster.pid))

    return cluster


def start_persistent_(persistence_config, cluster_config, mode=Mode.stable) -> ClusterState:
    name = cluster_config['name']
    state_dir = persistence_config.persistent_state_dir

    if name in persistence_config.state:
        cluster = persistence_config.state[name]
        # TODO : check for cluster_config == cluster.config
        if _is_running(cluster.pid):
            return cluster
        # TODO : cleanup FS

    pg_dir = os.path.join(state_dir, 'pg')
    if not os.path.exists(pg_dir):
        shutil.copytree(pg_resource_dir(), pg_dir)

    cluster = start_cluster_(
        config=cluster_config,
        mode=mode,
        pg_dir=pg_dir,
        work_dir=tempfile.mkdtemp(prefix=f'wd_{name}_', dir=state_dir),
        socket_dir=tempfile.mkdtemp(prefix='socket_', dir=state_dir),
    )
    persistence_config.state[cluster.name] = cluster
    persistence_config.write()
    persistence_config.make_shell_helper()

    return cluster


def start_cluster_(config, pg_dir, work_dir, socket_dir, mode=Mode.performance) -> ClusterState:
    if mode:
        if not config.get("config"):
            config["config"] = {}

        if mode == Mode.performance:
            for opt in ['fsync', 'full_page_writes', 'synchronous_commit', 'wal_log_hints']:
                config["config"].setdefault(opt, 'off')
        elif mode == Mode.stable:
            for opt in ['synchronous_commit', 'wal_log_hints']:
                config["config"].setdefault(opt, 'off')

    bin_dir = os.path.join(pg_dir, 'bin')
    lib_dir = os.path.join(pg_dir, 'lib')

    env = os.environ.copy()
    lib_path_var_name = 'DYLD_FALLBACK_LIBRARY_PATH' if sys.platform.startswith('darwin') else 'LD_LIBRARY_PATH'
    if lib_path_var_name in env:
        env[lib_path_var_name] += ':' + lib_dir
    else:
        env[lib_path_var_name] = lib_dir

    env['LANG'] = 'en_US.utf8'
    env['LC_MESSAGES'] = 'en_US.utf8'
    env['PYTHONHOME'] = pg_dir

    initdb_path = os.path.join(bin_dir, 'initdb')
    postgres_server_path = os.path.join(bin_dir, 'postgres')
    basebackup_path = os.path.join(bin_dir, 'pg_basebackup')

    # We use unix-sockets and we can run several processes on port 5432
    postgres_port = 5432

    replication_source = config.get('replication_source')

    if not replication_source:
        cmd = [initdb_path, '-k', '--auth=trust', '-D', work_dir]
        subprocess.check_call(
            cmd,
            env=env,
        )
    else:
        cmd = [basebackup_path, '-d', replication_source, '--write-recovery-conf', '-D', work_dir]
        subprocess.check_call(
            cmd,
            env=env,
        )

    postgres_config = config.get('config', dict())
    postgres_custom_variable = config.get('postgres_custom_variable', dict())
    for key, value in postgres_custom_variable.items():
        with open(f'{work_dir}/postgresql.conf', 'a') as f:
            f.write(f'{key}={value}\n')

    cmd = [
        postgres_server_path,
        '-D',
        work_dir,
        '-k',
        socket_dir,
        f'--port={postgres_port}',
        '-N{}'.format(postgres_config.get('max_connections', 10)),
        '-c',
        'listen_addresses=',
        '-c',
        'max_wal_senders=2',
    ]
    for key, value in postgres_config.items():
        cmd.extend(['-c', f'{key}={value}'])
    server = subprocess.Popen(
        cmd,
        env=env,
    )

    _wait_postgres_ready(socket_dir, postgres_port)

    dbname = config.get('db', 'postgres')
    if not replication_source:
        if dbname != 'postgres':
            run_sql(socket_dir, postgres_port, f'CREATE DATABASE {dbname}')
        if 'before_migration_sql' in config:
            run_sql(socket_dir, postgres_port, config['before_migration_sql'], dbname=dbname)
        if 'source_path' in config:
            _run_migrations(socket_dir, postgres_port, config, dbname)

    return ClusterState(
        name=config['name'],
        config=config,
        pid=server.pid,
        pg_dir=pg_dir,
        work_dir=work_dir,
        socket_dir=socket_dir,
        port=postgres_port,
    )


def _is_running(pid):
    try:
        os.kill(pid, 0)
    except OSError as err:
        if err.errno == errno.ESRCH:
            return False
    return True


def stop(name=None):
    """
    Stop running postmaster
    """
    stop_local_(name=name)
    stop_persistent_(name=name)


def stop_local_(name=None):
    if name:
        clusters = [_pid_path(name)]
    elif os.path.isdir(os.path.abspath(_pid_dir())):
        clusters = [os.path.join(_pid_dir(), pid_file_name) for pid_file_name in os.listdir(_pid_dir())]
    else:
        clusters = []
    for pid_path in clusters:
        if not os.path.exists(pid_path):
            continue
        with open(pid_path) as inp:
            pid = int(inp.read())

        stop_cluster_(pid)
        os.remove(pid_path)


def stop_persistent_(name=None):
    persistence_config = PersistenceConfig.read()
    if not persistence_config:
        return
    if name and name in persistence_config.state:
        clusters = [persistence_config.state[name]]
    else:
        clusters = [
            cluster
            for name, cluster in persistence_config.state.items()
            if name not in persistence_config.persist_clusters
        ]
    for cluster in clusters:
        stop_cluster_(cluster.pid)
        # TODO : cleanup FS
        del persistence_config.state[cluster.name]
        persistence_config.write()
        persistence_config.make_shell_helper()


def stop_cluster_(pid):
    if _is_running(pid):
        os.kill(pid, 15)
    timeout = 10
    for _ in range(timeout):
        if not _is_running(pid):
            break
        time.sleep(1)
    if _is_running(pid):
        logging.error('postmaster is still running after %s seconds', timeout)
        os.kill(pid, 9)
