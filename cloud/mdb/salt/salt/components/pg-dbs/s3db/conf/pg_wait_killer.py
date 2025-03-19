"""
PostgreSQL bad wait_event watcher
"""

import time
import traceback
from contextlib import suppress
from logging import DEBUG, Formatter, getLogger
from logging.handlers import RotatingFileHandler

from psycopg2 import connect
from psycopg2.extras import NamedTupleCursor

_initialized = False

BAD_SESSIONS = """
SELECT count(*) sessions
FROM pg_stat_activity
WHERE wait_event IN
      ('SubtransControlLock',
      'MultiXactGenLock',
      'MultiXactOffsetControlLock',
      'MultiXactMemberControlLock',
      'subtrans',
      'multixact_offset',
      'multixact_member');
"""

KILL_SESSIONS = """
SELECT mdb_terminate_backend(pid), pid, query, wait_event, usename
FROM pg_stat_activity
WHERE backend_type IN ('client backend', 'autovacuum worker')
AND pid!=pg_backend_pid()
"""


class PostgresqlWaitKiller:
    """
    Check wait session and kill them
    """

    def __init__(self):
        self.config = None
        self.log = None
        self.pg_conn = None
        self._window_size = None
        self._sessions = None
        self._last_kill_ts = None

    def pg_connect(self):
        """
        (Re-)init connection with database
        """
        if self.pg_conn is not None:
            with suppress(Exception):
                self.pg_conn.close()
        self.pg_conn = connect(self.config['conn_string'])

    def initialize(self, config):
        """
        Init loggers and required connections
        """
        global _initialized  # pylint: disable=global-statement

        if _initialized:
            return  # pragma: nocover

        self.config = config
        self._window_size = self.config['window_size']
        # Common logger
        self._sessions = [0] * self._window_size
        self.log = getLogger(__name__ + '.service')
        self.log.setLevel(DEBUG)
        service_handler = RotatingFileHandler(
            self.config['service_log_file'],
            maxBytes=self.config['rotate_size'],
            backupCount=self.config['backup_count'])
        service_handler.setFormatter(Formatter(fmt='%(asctime)s [%(levelname)s]: %(message)s'))
        self.log.addHandler(service_handler)
        _initialized = True

    def check(self):
        """
        Check wait session and kill them
        """
        if not self.pg_conn:
            self.pg_connect()
        try:
            with self.pg_conn as txn:
                cursor = txn.cursor(cursor_factory=NamedTupleCursor)
                cursor.execute(BAD_SESSIONS)
                result = cursor.fetchone()
                self._sessions = self._sessions[1:] + [result[0]]
                total = sum(self._sessions)
                self.log.info('Sessions: %s', self._sessions)
                if total > self.config['sum_session_to_kill']:
                    if self._last_kill_ts is not None \
                            and self._last_kill_ts > time.time()-self.config['min_kill_timeout']:
                        kill_sec = int(time.time()-self._last_kill_ts)
                        self.log.error('Last session kill was: %s seconds ago, skipping', kill_sec)
                    else:
                        self.log.info('Start to kill bad sessions')
                        cursor.execute(KILL_SESSIONS)
                        result = cursor.fetchall()
                        self._last_kill_ts = time.time()
                        for row in result:
                            self.log.info('Kill pid: %s, user: %s, query: %s, wait_event: %s',
                                          row.pid, row.usename, row.query, row.wait_event)
        except Exception:
            self.log.error('Unable to check sessions: %s', traceback.format_exc())
            self.pg_connect()


KILLER = PostgresqlWaitKiller()


def pg_wait_killer(config):
    """
    Run postgresql wait_event check (use this as dbaas-cron target function)
    """
    if not _initialized:
        KILLER.initialize(config)

    KILLER.check()
