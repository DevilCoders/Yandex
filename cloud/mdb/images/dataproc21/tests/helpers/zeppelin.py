"""
Helpers for Apache Zeppelin
For detailed information about REST API see
https://zeppelin.apache.org/docs/0.9.0/usage/rest_api/notebook.html#overview
"""

import logging
import requests

from helpers import tunnel

LOG = logging.getLogger('zeppelin')
PORT = 8890
TIMEOUT = 15.0


def notebooks_list(ctx):
    """
    List of the notes
    https://zeppelin.apache.org/docs/0.9.0/usage/rest_api/notebook.html#list-of-the-notes
    """
    endpoint = tunnel.tunnel_get(ctx, PORT)
    r = requests.get(
        f'http://{endpoint}/api/notebook',
        headers={'Content-Type': 'application/json'},
        timeout=TIMEOUT,
    )
    r.raise_for_status()
    return r.json()


def paragraphs_list(ctx, notebook_id: str):
    """
    Get the status of all paragraphs
    https://zeppelin.apache.org/docs/0.9.0/usage/rest_api/notebook.html#get-the-status-of-all-paragraphs
    """
    endpoint = tunnel.tunnel_get(ctx, PORT)
    r = requests.get(
        f'http://{endpoint}/api/notebook/job/{notebook_id}',
        headers={'Content-Type': 'application/json'},
        timeout=TIMEOUT,
    )
    r.raise_for_status()
    return r.json()


def paragraph_info(ctx, notebook_id: str, paragraph_id: str):
    """
    Get a paragraph information
    https://zeppelin.apache.org/docs/0.9.0/usage/rest_api/notebook.html#get-a-paragraph-information
    """
    endpoint = tunnel.tunnel_get(ctx, PORT)
    r = requests.get(
        f'http://{endpoint}/api/notebook/{notebook_id}/paragraph/{paragraph_id}',
        headers={'Content-Type': 'application/json'},
        timeout=TIMEOUT,
    )
    r.raise_for_status()
    return r.json()


def paragraph_run(ctx, notebook_id: str, paragraph_id: str, timeout: float = 300.0):
    """
    Run a paragraph synchronously
    https://zeppelin.apache.org/docs/0.9.0/usage/rest_api/notebook.html#run-a-paragraph-synchronously
    """
    endpoint = tunnel.tunnel_get(ctx, PORT)
    r = requests.post(
        f'http://{endpoint}/api/notebook/run/{notebook_id}/{paragraph_id}',
        headers={'Content-Type': 'application/json'},
        timeout=timeout,
    )
    r.raise_for_status()
    return r.json()
