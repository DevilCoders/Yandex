{% from "components/postgres/pg.jinja" import pg with context %}
"""
PostgreSQL activity reporter
"""

import json
import socket
from datetime import datetime
from contextlib import suppress
from logging import DEBUG, Formatter, getLogger
from logging.handlers import MemoryHandler, RotatingFileHandler

from psycopg2 import connect
from psycopg2.extras import RealDictCursor

_initialized = False

PG_STAT_ACTIVITY_QUERY = """
SELECT
    datname,
    pid,
    COALESCE(usename, 'postgres') usename,
    application_name,
    client_addr,
    client_hostname,
    client_port,
    (backend_start::timestamp without time zone)::text,
    (xact_start::timestamp without time zone)::text,
    (query_start::timestamp without time zone)::text,
    (state_change::timestamp without time zone)::text,
    coalesce(wait_event_type,'CPU') wait_event_type,
    coalesce(wait_event,'CPU') wait_event,
    state,
    backend_xid,
    backend_xmin,
    query,
    backend_type,
    (pg_blocking_pids(pid)) blocking_pids,
{% if pg.version.major_num >= 1400 %}
    COALESCE(query_id, 0)::text queryid
{% else %}
    COALESCE(queryid, 0)::text queryid
{% endif %}
FROM pg_stat_activity
{% if pg.version.major_num < 1400 %}
LEFT JOIN mdb_pid_queryid USING (pid)
{% endif %}
WHERE pid != pg_backend_pid()
AND backend_type IN ('client backend', 'autovacuum worker')
AND state != 'idle'
{% if pg.version.major_num < 1400 %}
AND COALESCE(ts, now()) > backend_start
{% endif %}
AND (datname != 'postgres' AND datname NOT LIKE 'template%')
AND (usename NOT IN ('postgres', 'monitor', 'admin') AND backend_type='client backend');
"""


class PostgresqlStatActivityFormatter(Formatter):
    """
    Pg_stat_activity record formatter
    """

    def format(self, record):
        """
        Format record pg_stat_activity for pushclient
        """
        fields = list(vars(record))
        data = {name: getattr(record, name, None) for name in fields}

        ret = dict(
            collect_time=data['params']['collect_ts'],
            cluster_id=data['params']['cluster_id'],
            cluster_name=data['params']['cluster_name'],
            host=data['params']['fqdn'],
            database=data['stat']['datname'],
            pid=data['stat']['pid'],
            user=data['stat']['usename'],
            application_name=data['stat']['application_name'],
            client_addr=data['stat']['client_addr'],
            client_hostname=data['stat']['client_hostname'],
            client_port=data['stat']['client_port'],
            backend_start=data['stat']['backend_start'],
            xact_start=data['stat']['xact_start'],
            query_start=data['stat']['query_start'],
            state_change=data['stat']['state_change'],
            wait_event_type=data['stat']['wait_event_type'],
            wait_event=data['stat']['wait_event'],
            state=data['stat']['state'],
            backend_xid=data['stat']['backend_xid'],
            backend_xmin=data['stat']['backend_xmin'],
            query=data['stat']['query'],
            backend_type=data['stat']['backend_type'],
            blocking_pids=data['stat']['blocking_pids'],
            queryid=data['stat']['queryid'])
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
        self._cluster_name = None
        self._initialized = False

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
        self._cluster_name = self.config['cluster_name']
        # Logger for pg_stat_activity
        self.activity_log = getLogger(__name__ + '.activity')
        self.activity_log.setLevel(DEBUG)
        activity_file_handler = RotatingFileHandler(
            self.config['activity_log_file'],
            maxBytes=self.config['rotate_size'],
            backupCount=self.config['backup_count'])
        activity_formatter = PostgresqlStatActivityFormatter()
        activity_file_handler.setFormatter(activity_formatter)
        self.activity_handler = MemoryHandler(1000, 50, activity_file_handler)
        self.activity_log.addHandler(self.activity_handler)
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
        params = {'collect_ts': datetime.now().strftime('%Y-%m-%d %H:%M:%S'), 'fqdn': self._fqdn, 'cluster_id': self._cluster_id, 'cluster_name': self._cluster_name}
        if not self.pg_conn:
            self.pg_connect()
        try:
            with self.pg_conn as txn:
                cursor = txn.cursor()
                cursor.execute(PG_STAT_ACTIVITY_QUERY)
                result = cursor.fetchall()
                for row in result:
                    self.activity_log.info('', extra=dict(params=params, stat=row))
                self.activity_handler.flush()

        except Exception:
            self.log.exception('Unable to run report')
            self.pg_connect()


REPORTER = PostgresqlStatReporter()


def pg_stat_activity(config):
    """
    Run postgresql stat report (use this as dbaas-cron target function)
    """
    if not _initialized:
        REPORTER.initialize(config)

    REPORTER.report()

    REPORTER.log.info('DB statistics dump successfully')
