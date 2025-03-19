"""
Database helpers
"""

from pprint import pprint

from psycopg2 import OperationalError
from psycopg2.extensions import make_dsn, parse_dsn

from .queries import autocommit_query, execute_query


def _print_client_sessions(dsn):
    with execute_query(
        dsn,
        """SELECT *
                 FROM pg_stat_activity
                WHERE pid != pg_backend_pid()
                  AND backend_type ~ 'client'""",
    ) as res:
        for row in res.fetch():
            pprint(row)  # noqa


def _drop_if_exists(dbname: str, adm_dsn: str) -> None:
    with execute_query(adm_dsn, 'SELECT count(*) FROM pg_database WHERE datname=%s', [dbname]) as query_result:
        if query_result.fetch()[0].count == 0:
            return
    try:
        autocommit_query(adm_dsn, f'DROP DATABASE {dbname}')
    except OperationalError:
        _print_client_sessions(adm_dsn)
        raise


def recreate_database(dbname: str, adm_dsn: str) -> str:
    """
    Recreate database
    """
    _drop_if_exists(dbname, adm_dsn)
    autocommit_query(adm_dsn, f'CREATE DATABASE {dbname}')


def make_dsn_to_database(dbname: str, dsn: str) -> str:
    """
    Return DSN that points to dbname
    """
    parsed = parse_dsn(dsn)
    parsed['dbname'] = dbname
    return make_dsn(**parsed)
