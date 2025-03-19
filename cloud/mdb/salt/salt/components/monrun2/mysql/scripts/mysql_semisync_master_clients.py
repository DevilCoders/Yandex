#!/usr/bin/python
"""
Check MySQL master semisync replication
"""

import json
import os
import sys
from mysql_util import die, is_offline, connect


CRITICAL_SEMI_SYNC_MASTER_CLIENTS_NUMBER = 0


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

            semi_sync_client_number = int(getter('Rpl_semi_sync_master_clients'))

            if semi_sync_client_number <= CRITICAL_SEMI_SYNC_MASTER_CLIENTS_NUMBER:
                die(2, "{0} semi sync clients".format(semi_sync_client_number))

            die()
    except Exception:
        if is_offline():
            die(1, 'Mysql is offline')
        die(1, 'Mysql is dead')


if __name__ == '__main__':
    _main()
