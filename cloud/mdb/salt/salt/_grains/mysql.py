#!/usr/bin/env python

import json
import os
import re
import socket


def mysql():
    data = {}
    conn = None
    try:
        data['suffix'] = socket.gethostname().split('.')[0][:-1]

        import MySQLdb
        conn = MySQLdb.connect(db='mysql', read_default_file=os.path.expanduser("~mysql/.my.cnf"))
        cur = conn.cursor()

        # 5, 7, 23, 5.7.23-23-57-log
        cur.execute('SELECT sys.version_major(), sys.version_minor(), sys.version_patch(), version()')
        major_num, minor_num, patch_num, full = cur.fetchone()
        data['version'] = {
            'full': full,
            'major': '{}.{}'.format(major_num, minor_num),
            'short': '{}{}'.format(major_num, minor_num),
        }
        version = (major_num, minor_num, patch_num)
        blm_version = (8, 0, 25)

        cur.execute("SELECT DISTINCT SCHEMA_NAME FROM information_schema.SCHEMATA "
                    "WHERE SCHEMA_NAME NOT IN "
                    "('mysql', 'sys', 'information_schema', 'performance_schema')")
        data['databases'] = [x[0] for x in cur.fetchall()]

        cur.execute("SELECT DISTINCT USER FROM mysql.user WHERE USER NOT IN "
                    "('root', 'mysql.sys', 'mysql.session', 'mysql.infoschema')")
        data['users'] = [x[0] for x in cur.fetchall()]

        with open('/tmp/.grains_mysql.cache', 'w') as cache:
            cache.write(json.dumps(data))

        #
        # We don't want to cache this part so we do it after
        # saving cache file.
        #
        role = 'master'
        if version >= blm_version:
            query = 'SHOW REPLICA STATUS'
        else:
            query = 'SHOW SLAVE STATUS'
        cur.execute(query)
        for row in cur.fetchall():
            row = {cur.description[i][0]: c for i, c in enumerate(row)}
            role = 'replica'
            if version >= blm_version:
                data['master'] = row['Source_Host']
            else:
                data['master'] = row['Master_Host']
        data['role'] = role

    except Exception:
        if os.path.exists('/tmp/.grains_mysql.cache'):
            with open('/tmp/.grains_mysql.cache', 'r') as cache:
                data = json.loads(cache.read())
    finally:
        if conn:
            conn.close()

    return {'mysql': data}


if __name__ == '__main__':
    from pprint import pprint
    pprint(mysql())
