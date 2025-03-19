"""
PostgreSQL cluster helper module
"""
import os
import subprocess
import time

import psycopg2
from psycopg2.extras import RealDictCursor


class PostgresCluster:
    """
    PostgreSQL process/conn helper
    """

    def __init__(self, user, data_dir, bin_dir, port):
        self.user = user
        self.data_dir = data_dir
        self.bin_dir = bin_dir
        self.port = port
        self.proc = subprocess.Popen(
            [os.path.join(self.bin_dir, 'bin', 'postgres'), '-D', '.'],
            cwd=self.data_dir,
            stderr=subprocess.PIPE,
            stdout=subprocess.PIPE,
        )

    def get_dbs(self):
        """
        Get list of postgresql databases in cluster
        """
        tries = 0
        while tries < 30:
            try:
                with psycopg2.connect(
                    'user={user} port={port} dbname=postgres'.format(user=self.user, port=self.port)
                ) as conn:
                    cursor = conn.cursor()
                    cursor.execute(
                        """
                        SELECT datname FROM pg_database WHERE NOT datistemplate AND datname != 'postgres'
                        """
                    )
                    return [x[0] for x in cursor.fetchall()]
            except Exception:
                time.sleep(1)
                tries += 1

        raise Exception('Timeout waiting for PostgreSQL with data dir {data_dir}'.format(data_dir=self.data_dir))

    def get_conn(self, dbname):
        """
        Get postgresql connection with cluster
        """
        dsn = 'user={user} port={port} dbname={dbname}'.format(user=self.user, port=self.port, dbname=dbname)
        return psycopg2.connect(dsn, cursor_factory=RealDictCursor)

    def __del__(self):
        if self.proc.poll() is None:
            self.proc.terminate()
