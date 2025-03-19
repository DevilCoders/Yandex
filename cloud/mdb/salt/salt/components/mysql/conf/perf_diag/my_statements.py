"""
MySQL statements reporter
"""

import copy
import datetime
import decimal
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

STATEMENTS_QUERY = "select * from sys.mdb_statements;"

_prev = {}

_initialized = False


class MysqlStatementsFormatter(Formatter):
    """
    MySQL statements record formatter
    """

    def format(self, record):
        """
        Format record mysql statements for pushclient
        """
        fields = list(vars(record))
        data = {name: getattr(record, name, None) for name in fields}

        ret = dict(
            collect_time=data['params']['collect_ts'],
            cluster_id=data['params']['cluster_id'],
            host=data['params']['fqdn'],
            database=data['stat']['db'],
            digest=data['stat']['digest'],
            query=data['stat']['query'],
            calls=data['stat']['calls'],
            total_query_latency=float(data['stat']['total_query_latency']),
            total_lock_latency=float(data['stat']['total_lock_latency']),
            avg_query_latency=float(data['stat']['avg_query_latency']),
            avg_lock_latency=float(data['stat']['avg_lock_latency']),
            errors=data['stat']['errors'],
            warnings=data['stat']['warnings'],
            rows_affected=data['stat']['rows_affected'],
            rows_sent=data['stat']['rows_sent'],
            rows_examined=data['stat']['rows_examined'],
            tmp_disk_tables=data['stat']['tmp_disk_tables'],
            tmp_tables=data['stat']['tmp_tables'],
            select_full_join=data['stat']['select_full_join'],
            select_full_range_join=data['stat']['select_full_range_join'],
            select_range=data['stat']['select_range'],
            select_range_check=data['stat']['select_range_check'],
            select_scan=data['stat']['select_scan'],
            sort_merge_passes=data['stat']['sort_merge_passes'],
            sort_range=data['stat']['sort_range'],
            sort_rows=data['stat']['sort_rows'],
            sort_scan=data['stat']['sort_scan'],
            no_index_used=data['stat']['no_index_used'],
            no_good_index_used=data['stat']['no_good_index_used'],
        )
        return json.dumps(ret)


class MysqlStatReporter:
    """
    Mysql stat reporter
    """

    def __init__(self):
        self.config = None
        self.statements_log = None
        self.statements_handler = None
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
        # Logger for my_statements
        self.statements_log = getLogger(__name__ + '.statements')
        self.statements_log.setLevel(DEBUG)
        statements_file_handler = RotatingFileHandler(
            self.config['statements_log_file'],
            maxBytes=self.config['rotate_size'],
            backupCount=self.config['backup_count'])
        statements_formatter = MysqlStatementsFormatter()
        statements_file_handler.setFormatter(statements_formatter)
        self.statements_handler = MemoryHandler(1000, 50, statements_file_handler)
        self.statements_log.addHandler(self.statements_handler)
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
        Query statements and write log
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
                cursor.execute(STATEMENTS_QUERY)
                result = cursor.fetchall()
                for row in result:
                    key = (row['db'], row['digest'])
                    prev_row = _prev.get(key)
                    if prev_row is not None and prev_row['calls'] < row['calls']:
                        diff = {}
                        has_changes = 0
                        for k in row:
                            if isinstance(row[k], (int, float, decimal.Decimal)):
                                diff[k] = row[k] - prev_row[k]
                                has_changes += diff[k]
                            else:
                                diff[k] = row[k]
                        if has_changes > 0:
                            diff['avg_query_latency'] = diff['total_query_latency'] / diff['calls']
                            diff['avg_lock_latency'] = diff['total_lock_latency'] / diff['calls']
                            self.statements_log.info('', extra=dict(params=params, stat=diff))
                    _prev[key] = copy.deepcopy(row)
            self.statements_handler.flush()
        except Exception:
            self.log.exception('Unable to run report')
            self.connect()


REPORTER = MysqlStatReporter()


def my_statements(config):
    """
    Run mysql perf report (use this as dbaas-cron target function)
    """
    if not _initialized:
        REPORTER.initialize(config)

    REPORTER.report()

    REPORTER.log.info('DB statistics dump successfully')
