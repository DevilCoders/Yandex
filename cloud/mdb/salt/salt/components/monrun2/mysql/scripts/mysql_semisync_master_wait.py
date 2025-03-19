#!/usr/bin/python
"""
Check MySQL master semisync replication
"""

import json
import os
import sys
from mysql_util import die, is_offline, connect


WARNING_SEMI_SYNC_MAASTER_TX_AVG_WAIT_TIME = 5 * 10 ** 5
CRITICAL_SEMI_SYNC_MASTER_TX_AVG_WAIT_TIME = 100 * 10 ** 5


def status_variable_value_getter(cur):

    def inner(variable_name):
        cur.execute("SHOW STATUS LIKE '{name}'".format(name=variable_name))
        res = cur.fetchone()

        return res[1]

    return inner

def _main():
    try:
        with connect() as conn:
            cur = conn.cursor()
            getter = status_variable_value_getter(cur)

            if getter('Rpl_semi_sync_master_status') != 'ON':
                die()

            semi_sync_master_tx_avg_wait_time = int(getter('Rpl_semi_sync_master_tx_avg_wait_time'))

            if semi_sync_master_tx_avg_wait_time >= CRITICAL_SEMI_SYNC_MASTER_TX_AVG_WAIT_TIME:
                die(2, "{val} semi sync master tx avg wait time".format(val=semi_sync_master_tx_avg_wait_time))

            if semi_sync_master_tx_avg_wait_time >= WARNING_SEMI_SYNC_MAASTER_TX_AVG_WAIT_TIME:
                die(1, "{val} semi sync master tx avg wait time".format(val=semi_sync_master_tx_avg_wait_time))

            semi_sync_master_wait_sessions = int(getter('Rpl_semi_sync_master_wait_sessions'))
            thread_connected = int(getter('Threads_connected'))

            if semi_sync_master_wait_sessions > 0.9 * thread_connected:
                die(2, "{val} semi sync master wait sessions".format(val=semi_sync_master_wait_sessions))

            if semi_sync_master_wait_sessions > 0.5 * thread_connected:
                die(1, "{val} semi sync master wait sessions".format(val=semi_sync_master_wait_sessions))

            die()
    except Exception:
        if is_offline():
            die(1, 'Mysql is offline')
        die(1, 'Mysql is dead')

if __name__ == '__main__':
    _main()
