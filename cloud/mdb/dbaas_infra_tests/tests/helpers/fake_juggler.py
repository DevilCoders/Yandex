"""
Fake juggler helpers
"""
import requests

from tests.helpers import docker


def _get_juggler_url(context):
    host, port = docker.get_exposed_port(
        docker.get_container(context, 'fake_juggler01'), context.conf['projects']['fake_juggler']['expose']['http'])

    return 'http://{host}:{port}'.format(host=host, port=port)


def set_failed(context, fqdn, service):
    """
    Set CRIT status for specified fqdn->service pair.
    """
    response = requests.post(
        _get_juggler_url(context) + '/internal/fail',
        headers={'Authorization': context.conf['projects']['fake_juggler']['config']['oauth']['token']},
        params={
            'host_name': fqdn,
            'service_name': service,
        })
    response.raise_for_status()
