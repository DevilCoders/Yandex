#!/usr/bin/env python3
"""
DBM Secrets expiration script
"""

import psycopg2


def expire_secrets():
    """
    Connect to local postgresql and clean expired secrets
    """
    with psycopg2.connect('dbname=dbm host=localhost') as conn:
        cursor = conn.cursor()
        cursor.execute('SELECT pg_is_in_recovery()')
        if not cursor.fetchone()[0]:
            cursor.execute(
                """
                UPDATE mdb.containers SET secrets = NULL, secrets_expire = NULL WHERE secrets_expire < now()
                """)


if __name__ == '__main__':
    expire_secrets()
