#!/usr/bin/python3

import json
import os
import sys
import subprocess

from mysql_util import die, is_offline, connect

WALG_CMD = 'wal-g-mysql --config /etc/wal-g/wal-g.yaml st cat binlog_sentinel_005.json'


def peek_first_gtid_with_gap(gtid_archived):
    for gtid in gtid_archived.split(','):
        if gtid.count(':') > 1:  # "UUID:1-10:20-30" or "UUID:1-10:20"
            return gtid
    return None


def _main():
    # don't check replicas:
    try:
        with connect() as conn:
            cur = conn.cursor()
            cur.execute("SHOW SLAVE STATUS")
            row = cur.fetchone()
            if row:
                die(0, 'replica')
    except Exception:
        if is_offline():
            die(1, 'Mysql is offline')
        die(1, 'Mysql is dead')

    # Check for gap in GTIDs
    try:
        data = subprocess.check_output(WALG_CMD, shell=True, stderr=subprocess.DEVNULL)
        data = json.loads(data)
        gtid_archived = data['GtidArchived']
    except Exception:
        die(1, 'wal-g-mysql call failed')

    gtidset = peek_first_gtid_with_gap(gtid_archived)
    if gtidset is not None:
        die(2, 'uploaded binlogs have gaps in GTIDs. GtidArchived: ' + gtidset)
    die()


if __name__ == '__main__':
    _main()
