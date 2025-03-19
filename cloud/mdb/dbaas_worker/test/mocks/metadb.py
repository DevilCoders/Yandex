"""
Simple metadb mock
"""

import json

from cloud.mdb.dbaas_worker.internal.query import QUERIES

from .utils import handle_action


def find_query_name(query):
    """
    Reverse match query name by query text (simplifies metadb state definition)
    """
    for query_name, query_text in QUERIES.items():
        if query_text == query:
            return query_name

    raise RuntimeError(f'Unable to find query_name for {query}')


def fetchall(state):
    """
    Return stored result
    """
    if 'result' not in state['metadb']:
        raise RuntimeError('No saved result in state')
    return state['metadb'].pop('result')


def execute(state, mogrified):
    """
    Match query against known results and store result in state
    """
    if mogrified == 'show transaction_read_only':
        return

    decoded = json.loads(mogrified)
    for query in state['metadb']['queries']:
        if query['query'] == decoded['query']:
            matched = True
            keys = []
            # Use exact match for kwargs
            for key, value in query.get('kwargs', {}).items():
                if decoded['kwargs'].get(key, None) == value:
                    keys.append(key)
                    continue
                matched = False
                keys = []
            if matched:
                action_id = f'metadb-execute-{query["query"]}-{"-".join(sorted(keys))}'
                handle_action(state, action_id)
                state['metadb']['result'] = query.get('result', [])
                return

    raise RuntimeError(f'Unable to match {decoded["query"]} (kwargs: {decoded["kwargs"]}) with known queries')


def metadb(mocker, state):
    """
    Setup metadb mock
    """
    connect = mocker.patch('psycopg2.connect')
    cursor = connect.return_value.cursor.return_value

    # fetchone is used only for read-only check, so it is safe to mock it this way
    cursor.fetchone.return_value = {'transaction_read_only': 'off'}

    cursor.mogrify.side_effect = lambda query, kwargs: json.dumps(
        {'query': find_query_name(query), 'kwargs': kwargs}
    ).encode('utf-8')

    cursor.connection.get_dsn_parameters.return_value = {'host': 'mocked-metadb'}

    cursor.fetchall.side_effect = lambda: fetchall(state)

    cursor.execute.side_effect = lambda mogrified: execute(state, mogrified)
