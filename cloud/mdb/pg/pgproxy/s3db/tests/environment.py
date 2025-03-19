# coding: utf-8

import contextlib
import getpass
import logging
import os
from collections import namedtuple

import psycopg2
import psycopg2.extras

log = logging.getLogger(__name__)


class QueryResult(namedtuple('QueryResult', ['records', 'errcode', 'errmsg'])):
    pass


class Connection(object):
    def __init__(self, connstring):
        self.connstring = connstring
        self._conn = psycopg2.connect(
            self.connstring,
            connection_factory=psycopg2.extras.LoggingConnection
        )
        self._conn.initialize(log)
        self._conn.autocommit = True

    def __create_cursor(self):
        try:
            cursor = self._conn.cursor()
            return cursor
        except Exception:
            return None

    def __exec_query(self, q, **kwargs):
        cur = self.__create_cursor()
        self.errcode = None
        self.errmsg = None
        try:
            cur.execute(q, kwargs)
        except psycopg2.Error as e:
            self.errcode = e.pgcode
            self.errmsg = e.pgerror
        return cur

    def __get_names(self, cur):
        return [r[0].lower() for r in cur.description]

    def __plain_format(self, cur):
        names = self.__get_names(cur)
        for row in cur.fetchall():
            yield dict(zip(names, tuple(row)))

    def get(self, q, **kwargs):
        with contextlib.closing(self.__exec_query(q, **kwargs)) as cur:
            records = list(self.__plain_format(cur)) if self.errcode is None else []
            return QueryResult(
                records,
                self.errcode,
                self.errmsg
            )

    def get_func(self, name, **kwargs):
        arg_names = ', '.join('{0} => %({0})s'.format(k) for k in kwargs)
        q = 'SELECT * FROM {name}({arg_names})'.format(
            name=name,
            arg_names=arg_names,
        )
        res = self.get(q, **kwargs)
        log.info(res)
        return res

    def query(self, q, **kwargs):
        with contextlib.closing(self.__exec_query(q, **kwargs)):
            return QueryResult(
                [],
                self.errcode,
                self.errmsg
            )


class DictComposite(psycopg2.extras.CompositeCaster):
    def make(self, values):
        return dict(zip(self.attnames, values))


class ProxyConnection(Connection):
    def __init__(self, connstring):
        super(ProxyConnection, self).__init__(connstring)
        psycopg2.extensions.register_type(psycopg2.extensions.UNICODE, self._conn)
        psycopg2.extensions.register_type(psycopg2.extensions.UNICODEARRAY, self._conn)
        self.register_composite_type('s3.object_part')

    def register_composite_type(self, composite_name):
        psycopg2.extras.register_composite(
            name=composite_name,
            conn_or_curs=self._conn,
            factory=DictComposite,
        )


def before_all(context):
    context.HOST = os.environ.get('PROXY_HOST', 'localhost')
    context.DBNAME = os.environ.get('PROXY_DBNAME', 's3proxy')
    context.DBUSER = os.environ.get('DBUSER', getpass.getuser())
    context.CONNSTRING = {
        's3proxy': 'host=%s dbname=%s user=%s' % (
            context.HOST, context.DBNAME, context.DBUSER
        ),
        's3db01': os.environ.get('S3DB01', 'dbname=s3db01'),
        's3db02': os.environ.get('S3DB02', 'dbname=s3db02'),
        's3meta01': os.environ.get('S3META01', 'dbname=s3meta01'),
        's3meta02': os.environ.get('S3META02', 'dbname=s3meta02'),
        'pgmeta': os.environ.get('S3PGMETA', 'dbname=s3db')
    }
    context.RO_CONNSTRING = {
        's3db01r': os.environ.get('S3DB01R', 'dbname=s3db01r'),
    }

    context.connect = ProxyConnection(context.CONNSTRING['s3proxy'])
    context.pgmeta_connect = Connection(context.CONNSTRING['pgmeta'])

    context.meta_connects = [
        Connection(connstring)
        for dbname, connstring in sorted(context.CONNSTRING.iteritems())
        if dbname.startswith('s3meta')
    ]
    context.db_connects = [
        Connection(connstring)
        for dbname, connstring in sorted(context.CONNSTRING.iteritems())
        if dbname.startswith('s3db')
    ]
    context.ro_connection = Connection(context.RO_CONNSTRING['s3db01r'])


def before_scenario(context, scenario):
    # This dict will be used for store objects returned from
    # database. It needed because you may have steps associated
    # with several different objects in one scenario.
    context.objects = {}
    # Also we use this hook to clear context
    context.result = None
    context.data = None


def after_step(context, step):
    if context.config.userdata.getbool('debug') and step.status == 'failed':
        # -- ENTER DEBUGGER: Zoom in on failure location.
        # NOTE: Use IPython debugger, same for pdb (basic python debugger).
        try:
            import ipdb
        except ImportError:
            import pdb as ipdb
        ipdb.post_mortem(step.exc_traceback)
