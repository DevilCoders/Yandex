#!/usr/bin/env python
"""
Check if MySQL Slave_IO_Running and Slave_SQL_Running
"""

import os
import sys
from mysql_util import die, is_offline, connect


def _main():
    try:
        with connect() as conn:
            cur = conn.cursor()
            cur.execute("SHOW SLAVE STATUS")
            row = cur.fetchone()
            if not row:
                die(0, 'master')
            slave_io_running = 'not found'
            slave_sql_running = 'not found'
            for idx, column in enumerate(cur.description):
                if column[0] == 'Slave_IO_Running':
                    slave_io_running = row[idx]
                elif column[0] == 'Slave_SQL_Running':
                    slave_sql_running = row[idx]
            if slave_io_running == 'Yes' and slave_sql_running == 'Yes':
                die()
            else:
                msg = []
                if not slave_io_running:
                    msg.append('Slave_IO_Running: {value}'.format(value=slave_io_running))
                if not slave_sql_running:
                    msg.append('Slave_SQL_Running: {value}'.format(value=slave_sql_running))
                die(2, '; '.join(msg))

    except Exception:
        if is_offline():
            die(1, 'Mysql is offline')
        die(1, 'Mysql is dead')


if __name__ == '__main__':
    _main()

