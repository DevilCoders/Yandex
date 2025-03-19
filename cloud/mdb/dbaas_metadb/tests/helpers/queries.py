"""
Query helpers
"""
import re
from collections import namedtuple
from contextlib import contextmanager
from typing import Any, Iterator, List, Set, Tuple

from .connect import transaction

VARS_RE = re.compile(r'(?P<sep>[^:a-zA-Z0-9])(?P<var>:[\w_]+)', re.M)


def translate_query_vars(query: str) -> Tuple[str, List[str]]:
    """
    Convert psql style variables to psycopg2 style

    Returns query with psycopg2 style variables and their names
    """

    args: Set[str] = set()

    def _translate_and_save(match):
        args.add(match.group('var'))
        var_style = '%({0})s'.format(match.group('var').lstrip(':'))
        return match.group('sep') + var_style

    tr_query = VARS_RE.sub(_translate_and_save, query)
    return tr_query, [a.lstrip(':') for a in args]


def strip_query(query: str) -> str:
    """
    Replace new lines with spaces, and strip duplicate spaces
    """
    return re.sub(r'\s+ ', ' ', query.replace('\n', ' ')).strip()


def verbose_query(query: str) -> str:
    """
    Convert psql style variables to psycopg2 style
    """
    return translate_query_vars(query)[0]


class QueryResult:  # pylint: disable=too-few-public-methods
    """Wrapper around psycopg.cursor"""

    def __init__(self, cur: object) -> None:
        self.cur = cur

    def fetch(self) -> List[tuple]:
        """
        Fetch cursor into list of namedtuples
        """
        # UPDATE, DELETE without RETURNING don't returns rows
        if self.cur.description is None:
            return []
        columns = [re.sub(r'\s+', '_', c.name) for c in self.cur.description]  # type: ignore
        ret_type = namedtuple('Row', columns)  # type: ignore
        return [ret_type(*r) for r in self.cur.fetchall()]  # type: ignore


def execute_query_in_transaction(conn: Any, query: str, query_vars: dict = None) -> QueryResult:
    """
    Execute query in transaction
    """
    cur = conn.cursor()
    cur.execute(strip_query(query), query_vars)
    return QueryResult(cur)


@contextmanager
def execute_query(dsn: str, query: str, query_vars: dict = None) -> Iterator[QueryResult]:
    """
    Execute query, return cursors
    """
    with transaction(dsn) as conn:
        yield execute_query_in_transaction(conn, query, query_vars)


def autocommit_query(dsn, query):
    """
    Execute query with autocommit
    """
    with transaction(dsn, autocommit=True) as conn:
        with conn.cursor() as cur:
            cur.execute(query)
