"""
Steps related to KMIP.
"""

from behave import given

from tests.helpers import docker
from tests.helpers.workarounds import retry


@given('kmip is up and running')
@given('PyKMIP is up and running')
@retry(wait_fixed=200, stop_max_attempt_number=25)
def wait_for_pykmip_alive(context):
    """
    Wait until kmip is ready to accept incoming requests.
    """
    kmip_container = docker.get_container(context, 'pykmip01')
    _, output = kmip_container.exec_run('python3 /etc/pykmip/ping_kmip.py')
    output = output.decode()
    if 'PASSED' not in output:
        raise RuntimeError('PyKMIP is not available: ' + output)
