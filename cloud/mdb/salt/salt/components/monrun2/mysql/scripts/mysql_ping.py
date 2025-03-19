#!/usr/bin/env python
"""
Check if local MySQL responds to ping
"""

import os
import socket
import sys
from mysql_util import die, is_offline, connect


def _main():
    try:
        with connect() as conn:
            cur = conn.cursor()
            cur.execute("SELECT 1")
            if cur.fetchall()[0][0] == 1:
                die()
    except Exception:
        pass
    if is_offline():
        die(2, 'Mysql is offline')
    die(2, 'Mysql is dead')


if __name__ == '__main__':
    _main()

