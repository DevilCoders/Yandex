#!/usr/bin/env python
"""
Check if local MySQL has users with Super_priv (except the 'admin')
"""

import os
import socket
import sys
from mysql_util import die, is_offline, connect


def _main():
    try:
        with connect() as conn:
            cur = conn.cursor()
            cur.execute("""select distinct User from mysql.user
                            where Super_priv = 'Y' and not
                            (User = 'admin' or
                            (User in ('mysql.session', 'root', 'mysql.sys') and Host = 'localhost'))""")

            users = ["'{0}'".format(row[0]) for row in cur.fetchall()]
            len_users = len(users)

            if len_users == 0:
                die()
            elif len_users == 1:
                die(2, "User {users} has SUPER privilege".format(users=users[0]))
            else:
                die(2, "Users {users} has SUPER privilege".format(users=",".join(users)))
    except Exception:
        if is_offline():
            die(1, 'Mysql is offline')
        die(1, 'Mysql is dead')


if __name__ == '__main__':
    _main()
