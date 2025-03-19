import logging
import sys
from copy import copy

import psycopg2.errors
import psycopg2.extensions
import sqlparse
import sshtunnel
import re
from click import ClickException
from jinja2 import Environment
from psycopg2.extras import DictCursor, Json

from cloud.mdb.cli.dbaas.internal import vault
from cloud.mdb.cli.dbaas.internal.config import get_config, get_environment_name
from cloud.mdb.cli.dbaas.internal.utils import debug_mode, dry_run_mode, get_free_port
from cloud.mdb.cli.dbaas.internal.version import is_dev_version
from tenacity import retry, retry_if_exception_type, stop_after_attempt, wait_random_exponential

psycopg2.extensions.register_adapter(dict, Json)


class MultipleRecordsError(RuntimeError):
    def __init__(self):
        super().__init__("Found multiple records, but it's expected to get only one.")


class DbTransaction:
    def __init__(self, ctx, config_section):
        if ctx.obj['config']['prod'] and is_dev_version():
            raise ClickException(
                'Production environment cannot be accessed using development version.'
                ' Please install official release version (https://nda.ya.ru/t/ygN3g53I3Y4WCp).'
            )

        self.ctx = ctx
        self.config_section = config_section
        self.txn_key = f'{self.config_section}_txn'
        self.active = False
        self.jumphost, self.conn = self._connect()
        self.enter_counter = 0

    def __del__(self):
        if self.jumphost:
            self.jumphost.close()

    def __enter__(self):
        self.enter_counter += 1
        if self.enter_counter == 1:
            self.conn.__enter__()
            self.ctx.obj[self.txn_key] = self
            self.active = True

        return self

    def __exit__(self, *args, **kwargs):
        self.enter_counter -= 1
        if self.enter_counter == 0:
            if dry_run_mode(self.ctx):
                self.rollback()

            self.active = False
            del self.ctx.obj[self.txn_key]
            self.conn.__exit__(*args, **kwargs)
            if self.jumphost:
                self.jumphost.__exit__()

    def query(self, query, *, fetch=True, fetch_single=False, **kwargs):
        assert self.active

        rendered_query = self._render_db_query(query, kwargs)

        with self.conn.cursor(cursor_factory=DictCursor) as cursor:
            mogrified_query = cursor.mogrify(rendered_query, kwargs)
            if debug_mode(self.ctx):
                print(sqlparse.format(mogrified_query, reindent=True), '\n', file=sys.stderr)

            try:
                cursor.execute(mogrified_query)
            except psycopg2.errors.InsufficientPrivilege as e:
                raise ClickException(str(e))

            if fetch:
                result = cursor.fetchall()

                if fetch_single:
                    if len(result) > 1:
                        raise MultipleRecordsError()
                    result = result[0] if result else None

                return result

    def rollback(self):
        if self.active:
            self.conn.rollback()
            self.active = False

    @retry(
        retry=retry_if_exception_type(psycopg2.OperationalError),
        wait=wait_random_exponential(multiplier=0.5, max=3),
        stop=stop_after_attempt(3),
        reraise=True,
    )
    def _connect(self):
        tunnel_config, dsn = get_dsn(self.ctx, self.config_section)
        jumphost_conn = None
        if tunnel_config:
            sshtunnel.SSH_TIMEOUT = 10
            jumphost_conn = sshtunnel.open_tunnel(**tunnel_config)
            jumphost_conn.start()
        return jumphost_conn, psycopg2.connect(dsn)

    @staticmethod
    def _render_db_query(query, vars):
        template = Environment().from_string(query)
        return template.render(vars)


def db_query(ctx, config_section, query, *, fetch=True, fetch_single=False, **kwargs):
    """
    Execute DB query.

    :param ctx: Context.
    :param config_section: Configuration section determining database to execute query on.
    :param query: SQL query to execute.
    :param fetch: Whether to fetch result or not.
    :param fetch_single: Fetch and return single item. Raise an exception if the query returned more than one item.
    Return `None` if the query returned no items.
    :param kwargs: Query parameters.
    """
    txn = ctx.obj.get(f'{config_section}_txn')
    if txn:
        return txn.query(query, fetch=fetch, fetch_single=fetch_single, **kwargs)

    with DbTransaction(ctx, config_section) as txn:
        return txn.query(query, fetch=fetch, fetch_single=fetch_single, **kwargs)


def db_transaction(ctx, config_section):
    """
    Create and return DB transaction. If there is already transaction in progress, it will be reused.
    """
    txn = ctx.obj.get(f'{config_section}_txn')
    if txn:
        return txn
    else:
        return DbTransaction(ctx, config_section)


def get_dsn(ctx, config_section):
    """
    Get DSN for connecting to DB.
    """
    config = copy(get_config(ctx).get(config_section, {}))

    hosts = config.pop('hosts', None)
    if not hosts:
        raise ClickException(f'Database "{config_section}" is not available in "{get_environment_name(ctx)}".')
    if isinstance(hosts, list):
        hosts = ','.join(hosts)
    else:
        hosts = hosts.replace(' ', ',')

    dsn = config.pop('dsn', None)
    if not dsn:
        dbname = config.pop('dbname', config_section)
        port = config.pop('port', 6432)
        user = config.pop('user')
        password = config.pop('password')
        if password.startswith('sec-'):
            password = vault.get_secret(password)['password']
        sslmode = config.pop('sslmode', 'require')
        connect_timeout = config.pop('connect_timeout', 2)
        dsn = f'port={port} dbname={dbname} user={user} password={password} sslmode={sslmode} connect_timeout={connect_timeout}'
        dsn += ''.join(f' {name}={value}' for name, value in config.items())

    jumphost_config = get_config(ctx).get('jumphost', {})
    tunnel_params = {}
    if jumphost_config:
        debug_level = logging.DEBUG if debug_mode(ctx) else logging.CRITICAL
        port = re.match(r'port=(?P<port>\d+)', dsn).group('port')
        bind_port = get_free_port()
        tunnel_params = {
            'ssh_address_or_host': (jumphost_config['host'], 22),
            'ssh_username': jumphost_config['user'],
            'remote_bind_address': (hosts.split(',')[0], int(port)),
            'local_bind_address': ('127.0.0.1', bind_port),
            'debug_level': debug_level,
        }
        hosts = 'localhost'
        dsn = re.sub(r'port=\d+', f'port={bind_port}', dsn)

    return tunnel_params, f'host={hosts} {dsn} target_session_attrs=read-write'
