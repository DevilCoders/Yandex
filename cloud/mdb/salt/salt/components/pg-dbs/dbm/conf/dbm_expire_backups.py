#!/usr/bin/env python3
"""
DBM Volume backups expiration script
"""

import argparse
import urllib.parse
from contextlib import closing

import psycopg2
import requests


def setup_session(base_url):
    """
    Setup requests session for connection reuse
    """
    session = requests.Session()
    retries = requests.adapters.Retry(total=5,
                                      backoff_factor=1,
                                      status_forcelist=[500, 502, 503, 504])
    adapter = requests.adapters.HTTPAdapter(pool_connections=1, pool_maxsize=1, max_retries=retries)
    parsed = urllib.parse.urlparse(base_url)
    session.mount(f'{parsed.scheme}://{parsed.netloc}', adapter)
    return session


def expire_backup(base_url, session, backup):
    """
    Expire single backup
    """
    try:
        session.delete(
            urllib.parse.urljoin(
                base_url,
                f'/api/v2/volume_backups_with_token/{backup["dom0"]}/{backup["container"]}'),
            params={'path': backup['path']},
            json={'token': backup['token']},
            verify='/opt/yandex/allCAs.pem')
    except Exception as exc:
        print(f'Unable to expire {backup["container"]}:{backup["path"]} on {backup["dom0"]}: {exc!r}')


def expire_backups():
    """
    Connect to local postgresql and clean expired backups
    """
    parser = argparse.ArgumentParser()

    parser.add_argument('-t',
                        '--threshold',
                        type=int,
                        required=True,
                        help='Backup expiration threshold (in seconds)')
    parser.add_argument('-b',
                        '--base-url',
                        type=str,
                        required=True,
                        help='DBM base url')

    args = parser.parse_args()

    with closing(psycopg2.connect('dbname=dbm host=localhost')) as conn:
        cursor = conn.cursor()
        cursor.execute('SELECT pg_is_in_recovery()')
        if cursor.fetchone()[0]:
            return
        cursor.execute(
            """
            SELECT vb.dom0, vb.container, vb.path, vb.delete_token
            FROM mdb.volume_backups vb
            JOIN mdb.dom0_hosts d ON (d.fqdn = vb.dom0)
            WHERE create_ts < now() - (%(threshold)s * interval '1 second')
            AND d.heartbeat >= current_timestamp - '5 minutes'::interval
            """,
            {'threshold': args.threshold},
        )
        backups = [{'dom0': x[0], 'container': x[1], 'path': x[2], 'token': x[3]} for x in cursor.fetchall()]

    session = setup_session(args.base_url)
    for backup in backups:
        expire_backup(args.base_url, session, backup)


if __name__ == '__main__':
    expire_backups()
