#!/usr/bin/env python
"""
Check MySQL available connections for system users
"""

import argparse
from mysql_util import die, is_offline, connect

import MySQLdb


SYSTEM_USERS = ('admin', 'monitor', 'repl')


def get_max_connections(cur):
    """
    Get per-user connection limits
    """
    cur.execute('''
        SELECT CONCAT(user, '@', host), max_user_connections
        FROM mysql.user
        WHERE user IN {}
    '''.format(SYSTEM_USERS))
    return {r[0]: r[1] for r in cur.fetchall()}


def get_user_connections(cur):
    """
    Get per-user current connections count
    """
    cur.execute('''
        SELECT CONCAT(processlist_user, '@', processlist_host), count(*)
        FROM performance_schema.threads
        WHERE processlist_user in {}
        AND CONNECTION_TYPE IS NOT NULL
        GROUP BY 1
    '''.format(SYSTEM_USERS))
    return {r[0]: r[1] for r in cur.fetchall()}


def format_msg(users, conns, limits):
    """
    Formats message for juggler
    """
    return ", ".join("user '{}' used {}/{} connections".format(u, conns[u], limits[u]) for u in users)


def _main():
    parser = argparse.ArgumentParser()

    parser.add_argument('-w', '--warn',
                        type=float,
                        default=0.5,
                        help='Warning limit in percent')

    parser.add_argument('-c', '--crit',
                        type=float,
                        default=0.2,
                        help='Critical limit')

    args = parser.parse_args()

    crits = []
    warns = []
    try:
        with connect() as conn:
            cur = conn.cursor()
            user_limits = get_max_connections(cur)
            user_conns = get_user_connections(cur)
            for user, limit in user_limits.items():
                if limit == 0:  # zero means no limits
                    continue
                connections = user_conns.get(user, 0)
                available_connections = limit - connections
                if available_connections <= args.crit * limit:
                    crits.append(user)
                elif available_connections <= args.warn * limit:
                    warns.append(user)
            if len(crits) > 0:
                die(2, format_msg(crits, user_conns, user_limits))
            elif len(warns) > 0:
                die(1, format_msg(warns, user_conns, user_limits))
            else:
                die(0, "OK")
    except Exception as err:
        if isinstance(err, MySQLdb.OperationalError) and err.args[0] == 1040:
            die(2, 'too many connections')
        if isinstance(err, MySQLdb.OperationalError) and err.args[0] == 1226:
            die(2, 'user \'monitor\' has exceeded the \'max_user_connections\'')
        if is_offline():
            die(1, 'Mysql is offline')
        die(1, 'Mysql is dead')


if __name__ == '__main__':
    _main()
