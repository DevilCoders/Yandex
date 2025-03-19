"""
Steps related to mlock.
"""

from behave import given, when

from tests.helpers import mlock
from tests.helpers.workarounds import retry


@given('Mlock api is up and running')
@retry(wait_fixed=1000, stop_max_attempt_number=30)
def mlock_ready(context):
    """
    Wait until mlock api is ready to accept incoming requests.
    """
    assert mlock.is_ready(context), 'Mlock api is down'


@when('we lock cluster')
@retry(wait_fixed=250, stop_max_attempt_number=40)
def lock_cluster(context):
    """
    Lock cluster hosts in mlock
    """
    hosts = [host['name'] for host in context.hosts]
    mlock.lock_cluster(context, hosts)


@when('we unlock cluster')
@retry(wait_fixed=250, stop_max_attempt_number=40)
def unlock_cluster(context):
    """
    Unlock cluster hosts in mlock
    """
    mlock.unlock_cluster(context)
