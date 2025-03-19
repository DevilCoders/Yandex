#!/usr/bin/env python3
"""
DBM Empty clusters expiration script
"""

import psycopg2


def expire_clusters():
    """
    Connect to local postgresql and clean empty clusters
    """
    with psycopg2.connect('dbname=dbm host=localhost') as conn:
        cursor = conn.cursor()
        cursor.execute('SELECT pg_is_in_recovery()')
        if not cursor.fetchone()[0]:
            cursor.execute(
                """
                DELETE FROM mdb.clusters WHERE name NOT IN (SELECT DISTINCT cluster_name FROM mdb.containers);
                """)


if __name__ == '__main__':
    expire_clusters()
