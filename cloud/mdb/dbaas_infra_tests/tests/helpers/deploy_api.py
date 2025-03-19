"""
Utilities for dealing with Deploy API
"""

import uuid

import requests

from tests.helpers import docker
from tests.helpers.workarounds import retry


class DeployAPIError(RuntimeError):
    """
    General API error exception
    """


def get_base_url(context):
    """
    Get base URL for sending requests to Internal API.
    """
    host, port = docker.get_exposed_port(
        docker.get_container(context, 'mdb-deploy-api01'),
        context.conf['projects']['mdb-deploy-api']['expose']['https'])

    return 'https://{0}:{1}'.format(host, port)


# pylint: disable=not-callable
@retry(wait_fixed=1000, stop_max_attempt_number=10)
def request(context, handle, method='GET', deserialize=False, **kwargs):
    """
    Perform request to Deploy API and check response on internal server
    errors (error 500).

    If deserialize is True, the response will also be checked on all
    client-side (4xx) and server-side (5xx) errors, and then deserialized
    by invoking Response.json() method.

    If 404 is encountered, None is returned and no retrying is done.
    """
    rid = str(uuid.uuid4())
    req_kwargs = {
        'headers': {
            'Accept': 'application/json',
            'Content-Type': 'application/json',
            'X-Request-Id': rid,
            'Authorization': 'OAuth {token}'.format(token=context.conf['deploy']['token']),
        },
        'timeout': (1, 2),
    }
    req_kwargs.update(kwargs)
    res = requests.request(
        method,
        '{base}/{handle}'.format(
            base=get_base_url(context),
            handle=handle,
        ),
        verify=False,
        **req_kwargs,
    )

    try:
        res.raise_for_status()
    except requests.HTTPError as exc:
        if res.status_code == 404:
            return None
        if deserialize or res.status_code >= 400:
            msg = '{0} {1} failed with {2}: {3}'.format(method, handle, res.status_code, res.text)
            raise DeployAPIError(msg) from exc

    return res.json() if deserialize else res
