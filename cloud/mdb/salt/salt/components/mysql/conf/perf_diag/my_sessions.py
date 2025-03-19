"""
MySQL sessions reporter
"""

import datetime
import json
import os
import socket

from contextlib import suppress
from logging import DEBUG, Formatter, getLogger
from logging.handlers import MemoryHandler, RotatingFileHandler

import pymysql
import pymysql.cursors


MYCNF_PATH = os.path.expanduser('~monitor/.my.cnf')

CONNECT_TIMEOUT = 5

SESSIONS_QUERY = "select * from sys.mdb_sessions;"

_initialized = False


class MysqlSessionsFormatter(Formatter):
    """
    MySQL sessions record formatter
    """

    def format(self, record):
        """
        Format record mysql sessions for pushclient
        """
        fields = list(vars(record))
        data = {name: getattr(record, name, None) for name in fields}

        def cast(v, t):
            if v is None:
                return None
            return t(v)

        ret = dict(
            collect_time=data['params']['collect_ts'],
            cluster_id=data['params']['cluster_id'],
            host=data['params']['fqdn'],
            thd_id=data['stat']['thd_id'],
            conn_id=data['stat']['conn_id'],
            database=data['stat']['db'],
            user=data['stat']['user'],
            command=data['stat']['command'],
            stage=data['stat']['stage'],
            stage_latency=cast(data['stat']['stage_latency'], float),
            query=data['stat']['query'],
            digest=data['stat']['digest'],
            query_latency=cast(data['stat']['query_latency'], float),
            lock_latency=cast(data['stat']['lock_latency'], float),
            current_wait=data['stat']['current_wait'],
            wait_object=data['stat']['wait_object'],
            wait_latency=cast(data['stat']['wait_latency'], float),
            trx_latency=cast(data['stat']['trx_latency'], float),
            current_memory=cast(data['stat']['current_memory'], int),
            client_addr=data['stat']['client_addr'],
            client_hostname=data['stat']['client_hostname'],
            client_port=data['stat']['client_port']
        )
        return json.dumps(ret)


class MysqlStatReporter:
    """
    Mysql stat reporter
    """

    def __init__(self):
        self.config = None
        self.sessions_log = None
        self.sessions_handler = None
        self.log = None
        self.conn = None
        self._fqdn = socket.getfqdn()
        self._cluster_id = None
        self._initialized = False

    def connect(self):
        """
        (Re-)init connection with database
        """
        if self.conn is not None:
            with suppress(Exception):
                self.conn.close()
        self.conn = pymysql.connect(read_default_file=MYCNF_PATH, db='mysql', connect_timeout=CONNECT_TIMEOUT)

    def initialize(self, config):
        """
        Init loggers and required connections
        """
        global _initialized  # pylint: disable=global-statement

        if _initialized:
            return  # pragma: nocover

        self.config = config
        self._cluster_id = self.config['cluster_id']
        # Logger for my_sessions
        self.sessions_log = getLogger(__name__ + '.sessions')
        self.sessions_log.setLevel(DEBUG)
        sessions_file_handler = RotatingFileHandler(
            self.config['sessions_log_file'],
            maxBytes=self.config['rotate_size'],
            backupCount=self.config['backup_count'])
        sessions_formatter = MysqlSessionsFormatter()
        sessions_file_handler.setFormatter(sessions_formatter)
        self.sessions_handler = MemoryHandler(1000, 50, sessions_file_handler)
        self.sessions_log.addHandler(self.sessions_handler)
        # Common logger
        self.log = getLogger(__name__ + '.service')
        self.log.setLevel(DEBUG)
        service_handler = RotatingFileHandler(
            self.config['service_log_file'],
            maxBytes=self.config['rotate_size'],
            backupCount=self.config['backup_count'])
        service_handler.setFormatter(Formatter(fmt='%(asctime)s [%(levelname)s]: %(message)s'))
        self.log.addHandler(service_handler)
        _initialized = True

    def report(self):
        """
        Query sessions and write log
        """
        params = {
            'collect_ts': datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S'),
            'fqdn': self._fqdn,
            'cluster_id': self._cluster_id
        }
        if not self.conn:
            self.connect()
        try:
            self.conn.ping()
            with self.conn.cursor(pymysql.cursors.DictCursor) as cursor:
                cursor.execute(SESSIONS_QUERY)
                result = cursor.fetchall()
                for row in result:
                    self.sessions_log.info('', extra=dict(params=params, stat=row))
            self.sessions_handler.flush()
        except Exception:
            self.log.exception('Unable to run report')
            self.connect()


REPORTER = MysqlStatReporter()


def my_sessions(config):
    """
    Run mysql perf report (use this as dbaas-cron target function)
    """
    if not _initialized:
        REPORTER.initialize(config)

    REPORTER.report()

    REPORTER.log.info('DB statistics dump successfully')
