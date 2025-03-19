"""
Fake SMTP helpers
"""

import requests

from tests.helpers import docker


def get_getter_url(context):
    """Get inbox getter url."""
    host, port = docker.get_exposed_port(
        docker.get_container(context, 'fake_smtp01'), context.conf['projects']['fake_smtp']['expose']['http'])
    return 'http://{host}:{port}/'.format(host=host, port=port)


def get_messages(context):
    """Get all incoming emails."""
    url = get_getter_url(context)
    return requests.get(url).json()
