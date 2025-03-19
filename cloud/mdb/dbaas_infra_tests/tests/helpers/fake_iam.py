"""
Fake iam helpers
"""

from tests.helpers import docker


def get_access_service_url(context):
    """
    Get access service control url
    """
    host, port = docker.get_exposed_port(
        docker.get_container(context, 'fake_iam01'), context.conf['projects']['fake_iam']['expose']['as-control'])

    return 'http://{host}:{port}'.format(host=host, port=port)


def get_identity_url(context):
    """
    Get identity url
    """
    host, port = docker.get_exposed_port(
        docker.get_container(context, 'fake_iam01'), context.conf['projects']['fake_iam']['expose']['identity'])

    return 'http://{host}:{port}/iam'.format(host=host, port=port)


def get_resource_manager_control_url(context):
    """
    Get resource manager control url
    """
    host, port = docker.get_exposed_port(
        docker.get_container(context, 'fake_resourcemanager01'),
        context.conf['projects']['fake_resourcemanager']['expose']['control'])

    return 'http://{host}:{port}'.format(host=host, port=port)
