#!/usr/bin/env python

import json
import os
import socket
import re


def pg():
    data = {}
    try:
        data['suffix'] = socket.gethostname().split('.')[0][:-1]

        import psycopg2
        user = 'monitor'
        password = ''

        with open(os.path.expanduser("~/.pgpass")) as pgpass:
            for line in pgpass:
                tokens = line.rstrip().split(':')
                if tokens[3] == user:
                    password = tokens[4]
                    break

        conn = psycopg2.connect("host=localhost port=5432 dbname=postgres " +
                                "user=%s password=%s " % (user, password) +
                                "connect_timeout=1")
        cur = conn.cursor()

        versions = {}
        cur.execute('SELECT version()')
        ver = cur.fetchone()[0].split()[1]
        versions['full'] = ver
        major_minor = re.search(r'^([0-9]+(\.[0-9])?)\.[0-9]', ver)
        if major_minor:
            versions['major'] = major_minor.groups(0)[0]
            versions['short'] = versions['major'].replace('.', '')
        elif 'beta' in ver:
            versions['major'] = ver[:ver.find('beta')]
            versions['short'] = versions['major']

        cur.execute("SELECT setting FROM pg_settings " +
                    "WHERE name = 'server_version_num'")
        val = int(cur.fetchone()[0])
        versions['server_version_num'] = val
        versions['major_version_num'] = val / 100

        data['version'] = versions

        cur.execute("SELECT datname FROM pg_database WHERE " +
                    "datistemplate = false AND datname != 'postgres'" +
                    "ORDER BY datname")
        data['databases'] = [x[0] for x in cur.fetchall()]

        cur.execute("SELECT rolname FROM pg_roles ORDER BY rolname")
        data['users'] = [x[0] for x in cur.fetchall()]

        cur.execute("SELECT slot_name from pg_replication_slots WHERE slot_type = 'physical'")
        data['replication_slots'] = [x[0] for x in cur.fetchall()]

        with open('/tmp/.grains_pg.cache', 'w') as cache:
            cache.write(json.dumps(data))

        #
        # We don't want to cache this part so we do it after
        # saving cache file.
        #
        cur.execute("SELECT pg_is_in_recovery()")
        if cur.fetchone()[0] == False:
            data['role'] = 'master'
        else:
            data['role'] = 'replica'

            try:
                cur.execute("SELECT master FROM repl_mon")
                data['master'] = cur.fetchone()[0]
            except Exception:
                pass

    except Exception:
        if os.path.exists('/tmp/.grains_pg.cache'):
            with open('/tmp/.grains_pg.cache', 'r') as cache:
                data = json.loads(cache.read())

    return { 'pg': data }


if __name__ == '__main__':
    from pprint import pprint
    pprint(pg())
