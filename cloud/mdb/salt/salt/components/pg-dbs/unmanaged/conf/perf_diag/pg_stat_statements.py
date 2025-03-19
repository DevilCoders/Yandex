{% from "components/postgres/pg.jinja" import pg with context %}
"""
PostgreSQL activity reporter
"""

import json
import socket
import time
from datetime import datetime
import copy
from contextlib import suppress
from logging import DEBUG, Formatter, getLogger
from logging.handlers import MemoryHandler, RotatingFileHandler

from psycopg2 import connect
from psycopg2.extras import RealDictCursor

_initialized = False
_pgss_prev_result = {}

PG_STAT_STATEMENTS_QUERY = """
SELECT
    rolname,
    datname,
    queryid::text as queryid,
    query,
    calls,
{% if pg.version.major_num < 1300 %}
    total_time,
    mean_time,
    min_time,
    max_time,
    stddev_time,
{% else %}
    total_exec_time AS total_time,
    mean_exec_time AS mean_time,
    min_exec_time AS min_time,
    max_exec_time AS max_time,
    stddev_exec_time AS stddev_time,
{% endif %}
    rows,
    shared_blks_hit,
    shared_blks_read,
    shared_blks_dirtied,
    shared_blks_written,
    local_blks_hit,
    local_blks_read,
    local_blks_dirtied,
    local_blks_written,
    temp_blks_read,
    temp_blks_written,
    blk_read_time,
    blk_write_time,
{% if pg.version.major_num < 1400 %}
    reads,
    writes,
    user_time,
    system_time
{% else %}
    exec_reads AS reads,
    exec_writes AS writes,
    exec_user_time AS user_time,
    exec_system_time AS system_time
{% endif %}
FROM pg_stat_statements s
    JOIN pg_stat_kcache() k USING (userid, dbid, queryid)
    JOIN pg_database d ON s.dbid = d.oid
    JOIN pg_roles r ON r.oid = userid
WHERE datname != 'postgres' AND datname NOT LIKE 'template%' and r.rolname not in ('postgres', 'monitor', 'admin')
"""


class PostgresqlStatStatementsFormatter(Formatter):
    """
    Pg_stat_statements record formatter
    """

    def format(self, record):
        """
        Format record pg_stat_statements for pushclient
        """
        fields = list(vars(record))
        data = {name: getattr(record, name, None) for name in fields}

        ret = dict(
            collect_time=data['params']['collect_ts'],
            cluster_id=data['params']['cluster_id'],
            host=data['params']['fqdn'],
            user=data['stat']['rolname'],
            database=data['stat']['datname'],
            queryid=data['stat']['queryid'],
            query=data['stat']['query'],
            calls=data['stat']['calls'],
            total_time=round(data['stat']['total_time'], 3),
            min_time=round(data['stat']['min_time'], 3),
            max_time=round(data['stat']['max_time'], 3),
            mean_time=round(data['stat']['mean_time'], 3),
            stddev_time=round(data['stat']['stddev_time'], 3),
            rows=data['stat']['rows'],
            shared_blks_hit=data['stat']['shared_blks_hit'],
            shared_blks_read=data['stat']['shared_blks_read'],
            shared_blks_dirtied=data['stat']['shared_blks_dirtied'],
            shared_blks_written=data['stat']['shared_blks_written'],
            local_blks_hit=data['stat']['local_blks_hit'],
            local_blks_read=data['stat']['local_blks_read'],
            local_blks_dirtied=data['stat']['local_blks_dirtied'],
            local_blks_written=data['stat']['local_blks_written'],
            temp_blks_read=data['stat']['temp_blks_read'],
            temp_blks_written=data['stat']['temp_blks_written'],
            blk_read_time=round(data['stat']['blk_read_time'], 3),
            blk_write_time=round(data['stat']['blk_write_time'], 3),
            reads=data['stat']['reads'],
            writes=data['stat']['writes'],
            user_time=round(data['stat']['user_time'], 3),
            system_time=round(data['stat']['system_time'], 3))
        return json.dumps(ret)


class PostgresqlStatReporter:
    """
    Postgresql stat reporter
    """

    def __init__(self):
        self.config = None
        self.activity_log = None
        self.activity_handler = None
        self.statements_log = None
        self.statements_handler = None
        self.log = None
        self.pg_conn = None
        self._fqdn = socket.getfqdn()
        self._cluster_id = None

    def pg_connect(self):
        """
        (Re-)init connection with database
        """
        if self.pg_conn is not None:
            with suppress(Exception):
                self.pg_conn.close()
        self.pg_conn = connect(self.config['conn_string'], cursor_factory=RealDictCursor)

    def initialize(self, config):
        """
        Init loggers and required connections
        """
        global _initialized  # pylint: disable=global-statement

        if _initialized:
            return  # pragma: nocover

        self.config = config
        self._cluster_id = self.config['cluster_id']
        # Logger for pg_stat_statements
        self.statements_log = getLogger(__name__ + '.statements')
        self.statements_log.setLevel(DEBUG)
        statements_file_handler = RotatingFileHandler(
            self.config['statments_log_file'],
            maxBytes=self.config['rotate_size'],
            backupCount=self.config['backup_count'])
        statements_formatter = PostgresqlStatStatementsFormatter()
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
        Query pg_stat_activity/pg_stat_statements and write log
        """
        params = {'collect_ts': datetime.now().strftime('%Y-%m-%d %H:%M:%S'),
                  'fqdn': self._fqdn, 'cluster_id': self._cluster_id}
        if not self.pg_conn:
            self.pg_connect()
        try:
            with self.pg_conn as txn:
                cursor = txn.cursor()
                cursor.execute(PG_STAT_STATEMENTS_QUERY)
                result = cursor.fetchall()
                pgss = {}
                diff_result = []
                global _pgss_prev_result
                for row in result:
                    row_copy = copy.deepcopy(row)
                    pgss[(row_copy.pop('rolname'), row_copy.pop('datname'), row_copy.pop('queryid'))] = row_copy
                    if _pgss_prev_result != {}:
                        try:
                            prev_result = _pgss_prev_result[(row['rolname'], row['datname'], row['queryid'])]
                        except KeyError:
                            continue
                        if prev_result['calls'] < row['calls']:
                            row['calls'] -= prev_result['calls']
                            row['total_time'] -= prev_result['total_time']
                            row['rows'] -= prev_result['rows']
                            row['shared_blks_hit'] -= prev_result['shared_blks_hit']
                            row['shared_blks_read'] -= prev_result['shared_blks_read']
                            row['shared_blks_dirtied'] -= prev_result['shared_blks_dirtied']
                            row['shared_blks_written'] -= prev_result['shared_blks_written']
                            row['local_blks_hit'] -= prev_result['local_blks_hit']
                            row['local_blks_read'] -= prev_result['local_blks_read']
                            row['local_blks_dirtied'] -= prev_result['local_blks_dirtied']
                            row['local_blks_written'] -= prev_result['local_blks_written']
                            row['temp_blks_read'] -= prev_result['temp_blks_read']
                            row['temp_blks_written'] -= prev_result['temp_blks_written']
                            row['blk_read_time'] -= prev_result['blk_read_time']
                            row['blk_write_time'] -= prev_result['blk_write_time']
                            row['reads'] -= prev_result['reads']
                            row['writes'] -= prev_result['writes']
                            row['user_time'] -= prev_result['user_time']
                            row['system_time'] -= prev_result['system_time']
                            diff_result.append(row)
                for row in diff_result:
                    self.statements_log.info('', extra=dict(params=params, stat=row))
                self.statements_handler.flush()
                _pgss_prev_result = pgss
        except Exception:
            self.log.exception('Unable to run report')
            self.pg_connect()


REPORTER = PostgresqlStatReporter()


def pg_stat_statements(config):
    """
    Run postgresql stat report (use this as dbaas-cron target function)
    """
    if not _initialized:
        REPORTER.initialize(config)

    REPORTER.report()

    REPORTER.log.info('DB statistics dump successfully')
