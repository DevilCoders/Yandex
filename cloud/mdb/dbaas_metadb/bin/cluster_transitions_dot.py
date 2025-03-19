"""
Make cluster status transitions dot
"""
import sys
from enum import Enum
from typing import NamedTuple

import psycopg2


class Function(Enum):
    """
    Functions enum
    """

    add = 'add'
    acquire = 'acquire'
    success = 'success'
    fail = 'fail'


class Transition(NamedTuple):  # pylint: disable=too-few-public-methods
    """
    Transition
    """

    from_status: str
    to_status: str
    action: str
    function: Function


def _get_transitions(conn):
    cur = conn.cursor()
    cur.execute(
        """
    SELECT from_status, to_status, action
      FROM code.cluster_status_add_transitions()
    """
    )
    for row in cur.fetchall():
        yield Transition(*row, function=Function.add)

    cur.execute(
        """
    SELECT from_status, to_status, action
      FROM code.cluster_status_acquire_transitions()
    """
    )
    for row in cur.fetchall():
        yield Transition(*row, function=Function.acquire)

    cur.execute(
        """
    SELECT from_status, to_status, action, result
      FROM code.cluster_status_finish_transitions()
    """
    )
    for row in cur.fetchall():
        func = Function.success if row[-1] else Function.fail
        yield Transition(*row[:-1], function=func)


COLORS = {
    Function.add: 'gray15',
    Function.acquire: 'gold3',
    Function.success: 'forestgreen',
    Function.fail: 'crimson',
}


def _print_transition(tr, fd):  # pylint: disable=invalid-name
    fd.write(' ' * 4)
    label = tr.function.value + '\\n' + tr.action
    color = COLORS[tr.function]
    fd.write(f'"{tr.from_status}" -> "{tr.to_status}"')
    fd.write(f'[color={color}, fontcolor={color}, label="{label}"]')
    fd.write(';\n')


HEADER = """
digraph G {
    node [shape=plaintext, fontsize=18];
"""

FOOTER = '}'


def _main():
    conn = psycopg2.connect('dbname=dbaas_metadb')
    sys.stdout.write(HEADER)
    for tr in _get_transitions(conn):  # pylint: disable=invalid-name
        _print_transition(tr, sys.stdout)
    sys.stdout.write(FOOTER)


if __name__ == "__main__":
    _main()
