"""
Secrets api utils
"""

from tests.helpers import docker


def get_base_url(context):
    """
    Get base URL for sending requests to Secrets API.
    """
    host, port = docker.get_exposed_port(
        docker.get_container(context, 'secrets01'), context.conf['projects']['secrets']['expose']['http'])

    return 'http://{0}:{1}'.format(host, port)
