#!/usr/bin/env python
# encoding: utf-8

import re
import logging
import psycopg2
from psycopg2.extensions import Xid, ISOLATION_LEVEL_REPEATABLE_READ
from util.exceptions import BadIsolationLevel


class Database(object):
    def __init__(self, connstring, user=None, password=None, autocommit=False,
                 shard_id=None, application_name=None):
        self.shard_id = shard_id
        self.user = user
        self.password = password
        self.autocommit = autocommit
        if user:
            connstring += " user=" + user
        if password:
            connstring += " password=" + password
        if application_name:
            connstring += " application_name=" + application_name
        self._connstring = connstring
        self._conn = psycopg2.connect(connstring)
        self._conn.autocommit = autocommit

    def set_session(self, isolation_level=None, readonly=None, deferrable=None, autocommit=None):
        self._conn.set_session(isolation_level=isolation_level, readonly=readonly,
                               deferrable=deferrable, autocommit=autocommit)

    def my_db(self):
        found = re.search("dbname=([^ ]+)", self._connstring)
        return found.group(1) if found else None

    def reconnect(self):
        self._conn.close()
        self.__init__(self._connstring, shard_id=self.shard_id,
                        autocommit=self.autocommit)

    def create_cursor(self, *args, **kwargs):
        return self._conn.cursor(*args, **kwargs)

    def is_master(self):
        cur = self.create_cursor()
        cur.execute("SELECT NOT pg_is_in_recovery();")
        return cur.fetchone()[0]

    def commit(self):
        self._conn.commit()

    def rollback(self):
        self._conn.rollback()

    def log_last_query(self, cur):
        logging.debug("'%s' query: %s", self._connstring, cur.query)

    def begin_tpc(self, name):
        self._prepared_xid = Xid.from_string(self.my_db() + "_" + name)
        self._conn.tpc_begin(self._prepared_xid)

    def prepare_tpc(self):
        self._conn.tpc_prepare()

    def commit_tpc(self, *args):
        self._conn.tpc_commit(*args)

    def rollback_tpc(self, *args):
        self._conn.tpc_rollback(*args)

    def get_prepared_xid(self):
        return self._prepared_xid

    def status(self):
        return self._conn.status

    def xact_advisory_lock(self, lock_num):
        cur = self.create_cursor()
        func = "" if isinstance(lock_num, int) else "hashtext"
        query = "SELECT pg_advisory_xact_lock({func}(%(lock_num)s))".format(func=func)
        cur.execute(query, {'lock_num': lock_num})
        self.log_last_query(cur)

    def get_prepared_xacts(self, delay=None):
        cur = self.create_cursor()
        cur.execute("""
            SELECT gid
            FROM pg_prepared_xacts
            WHERE %(delay)s IS NULL OR prepared < now() - %(delay)s::interval
        """, {'delay': delay})
        self.log_last_query(cur)
        return {x[0] for x in cur.fetchall()}

    def finish_prepared_xact(self, gid, commit):
        cur = self.create_cursor()
        if commit:
            cur.execute("COMMIT PREPARED %(gid)s", {'gid': gid})
        else:
            cur.execute("ROLLBACK PREPARED %(gid)s", {'gid': gid})
        self.log_last_query(cur)

    def ensure_repeatable_read(self):
        if self._conn.isolation_level != ISOLATION_LEVEL_REPEATABLE_READ:
            raise BadIsolationLevel('Not in repeatable read')

    def check_is_open(self):
        cur = self.create_cursor()
        cur.execute("SELECT current_setting('pgcheck.closed', true)::bool")
        result = cur.fetchone()
        return not (result and result[0])

    def get_state(self):
        # Returns (is_closed, is_replica, lag)
        cur = self.create_cursor()
        cur.execute("""
            SELECT
                current_setting('pgcheck.closed', true)::bool,
                pg_is_in_recovery(),
                extract(epoch FROM clock_timestamp() - pg_last_xact_replay_timestamp())
        """)
        return cur.fetchone()

    def close(self):
        cur = self.create_cursor()
        cur.execute("ALTER SYSTEM SET pgcheck.closed TO true")
        self.log_last_query(cur)
        cur.execute("SELECT pg_reload_conf()")
        self.log_last_query(cur)
        return True

    def open(self):
        cur = self.create_cursor()
        cur.execute("ALTER SYSTEM RESET pgcheck.closed")
        self.log_last_query(cur)
        cur.execute("SELECT pg_reload_conf()")
        self.log_last_query(cur)
        return True
