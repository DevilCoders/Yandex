"""
Helpers for Apache Livy
For detailed information about Livy REST API see
http://livy.apache.org/docs/latest/rest-api.html
"""

import json
import logging
import requests

from retrying import retry

from helpers import tunnel

LOG = logging.getLogger('livy-helper')
PORT = 8998
TIMEOUT = 15.0


class LivyException(Exception):
    """
    Common exception for Apache Livy helper
    """


@retry(wait_exponential_multiplier=1000, wait_exponential_max=10000, stop_max_attempt_number=5)
def sessions_list(ctx):
    """
    List all livy sesssions from REST service
    """
    livy_endpoint = tunnel.tunnel_get(ctx, PORT)
    r = requests.get(
        f'http://{livy_endpoint}/sessions',
        headers={'Content-Type': 'application/json'},
        timeout=TIMEOUT,
    )
    r.raise_for_status()
    return r.json()


def session_create(ctx, session_conf={'kind': 'spark', 'name': 'my_session'}):
    """
    Create new spark session through livy service
    """
    livy_endpoint = tunnel.tunnel_get(ctx, PORT)
    r = requests.post(
        f'http://{livy_endpoint}/sessions',
        headers={'Content-Type': 'application/json'},
        data=json.dumps(session_conf),
        timeout=TIMEOUT,
    )
    r.raise_for_status()
    return r.json()


@retry(wait_exponential_multiplier=1000, wait_exponential_max=10000, stop_max_attempt_number=5)
def session_get(ctx, session_id):
    """
    Get session informartion
    """
    livy_endpoint = tunnel.tunnel_get(ctx, PORT)
    r = requests.get(
        f'http://{livy_endpoint}/sessions/{session_id}',
        headers={'Content-Type': 'application/json'},
        timeout=TIMEOUT,
    )
    r.raise_for_status()
    return r.json()


def session_delete(ctx, session_id):
    """
    Delete livy sessionn
    """
    livy_endpoint = tunnel.tunnel_get(ctx, PORT)
    r = requests.delete(
        f'http://{livy_endpoint}/sessions/{session_id}',
        headers={'Content-Type': 'application/json'},
        timeout=TIMEOUT)
    r.raise_for_status()
    return r.json()


@retry(wait_exponential_multiplier=1000, wait_exponential_max=10000, stop_max_attempt_number=5)
def statements_list(ctx, session_id):
    """
    List all statements for specified session_id
    """
    livy_endpoint = tunnel.tunnel_get(ctx, PORT)
    r = requests.get(
        f'http://{livy_endpoint}/sessions/{session_id}/statements',
        headers={'Content-Type': 'application/json'})
    r.raise_for_status()
    return r.json()


def statement_create(ctx, session_id, code):
    """
    List all statements for specified session_id
    """
    livy_endpoint = tunnel.tunnel_get(ctx, PORT)
    r = requests.post(
        f'http://{livy_endpoint}/sessions/{session_id}/statements',
        headers={'Content-Type': 'application/json'},
        data=json.dumps({'code': code}),
    )
    r.raise_for_status()
    return r.json()


@retry(wait_exponential_multiplier=1000, wait_exponential_max=10000, stop_max_attempt_number=5)
def statement_get(ctx, session_id, statement_id):
    """
    List all statements for specified session_id
    """
    livy_endpoint = tunnel.tunnel_get(ctx, PORT)
    r = requests.get(
        f'http://{livy_endpoint}/sessions/{session_id}/statements/{statement_id}',
        headers={'Content-Type': 'application/json'},
    )
    r.raise_for_status()
    return r.json()
