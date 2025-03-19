#!/usr/bin/python

from contextlib import closing
import os
import time
import logging

import MySQLdb


ERROR_SLEEP = 10
SLAVE_SLEEP = 30


def get_conn():
    return MySQLdb.connect(read_default_file=os.path.expanduser("~mysql/.my.cnf"), db='mysql')


def repl_mon(conn):
    cur = conn.cursor()
    cur.execute('SET SESSION innodb_lock_wait_timeout = 5')
    while True:
        tick = time.time()
        logging.info('time %s', tick)
        cur.execute('SHOW SLAVE STATUS')
        if cur.fetchone():
            logging.info('slave')
            time.sleep(SLAVE_SLEEP)
            continue
        cur.execute('SELECT @@read_only')
        if int(cur.fetchone()[0]) > 0:
            logging.info('read-only')
            time.sleep(SLAVE_SLEEP)
            continue
        cur.execute("""
            INSERT INTO mysql.mdb_repl_mon (id, ts)
                (
                    SELECT 1, CURRENT_TIMESTAMP(3)
                    WHERE @@read_only = 0
                )
            ON DUPLICATE KEY UPDATE ts = CURRENT_TIMESTAMP(3)""")
        conn.commit()
        time.sleep(max(0, tick + 1.0 - time.time()))


while True:
    try:
        with closing(get_conn()) as conn:
            repl_mon(conn)
    except MySQLdb.Error as err:
        logging.exception(err)
        time.sleep(ERROR_SLEEP)
