import os
import sys
import subprocess
import warnings
import json

import MySQLdb


MYSYNC_CACHE_PATH = '/var/run/mysync/mysync.info'
BLM_VERSION = (8, 0, 25)


def die(status=0, message='OK'):
    """
    Print status in monrun-compatible format
    """
    print('{status};{message}'.format(status=status, message=message))
    sys.exit(0)


def is_offline():
    """
    Checks that mysql in offline mode
    """
    try:
        subprocess.check_call(['sudo', '-u', 'mysql', '/usr/local/yandex/monitoring/is_offline.sh'])
        return True
    except subprocess.CalledProcessError:
        return False


class ConnectionManager(object):

    def __init__(self, **kwargs):
        self.conn = None
        self.conn = MySQLdb.connect(**kwargs)

    def __enter__(self):
        warnings.filterwarnings('ignore', category=MySQLdb.Warning)
        return self.conn

    def __exit__(self, exc_type, exc_value, traceback):
        warnings.filterwarnings('default', category=MySQLdb.Warning)
        try:
            if self.conn is not None:
                self.conn.close()
        except Exception:
            pass


def connect(read_default_file=None, db='mysql', **kwargs):
    if read_default_file is None:
        read_default_file = os.path.expanduser("~monitor/.my.cnf")
    return ConnectionManager(read_default_file=read_default_file, db=db, **kwargs)


def get_master_fqdn():
    # master_host from SHOW SLAVE STATUS is not suitable for cascade replicas
    mysync_cache = json.load(open(MYSYNC_CACHE_PATH))
    return mysync_cache['master']


def is_replica(cursor):
    version = get_version(cursor)
    if version >= BLM_VERSION:
        query = 'SHOW REPLICA STATUS'
    else:
        query = 'SHOW SLAVE STATUS'
    cursor.execute(query)
    row = cursor.fetchone()
    return bool(row)


def is_readonly(cursor):
    cursor.execute("select @@read_only")
    row = cursor.fetchone()[0]
    return row


def get_version(cursor):
    cursor.execute('SELECT version()')
    # monitor user has no permission to call sys.version_major()
    # '8.0.25-15' -> (8, 0, 25)
    return tuple([int(s) for s in cursor.fetchone()[0].split('-')[0].split('.')])