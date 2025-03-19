"""
Steps related to Fake DBM.
"""

from behave import given
from retrying import retry

from tests.helpers import mock


@given('Fake DBM is up and running')
@given('up and running Fake DBM')
@retry(wait_fixed=250, stop_max_attempt_number=80)
def wait_for_fake_dbm(context):
    """
    Wait until fake_dbm is ready to accept incoming requests.
    """
    mclient = mock.MockClient(context, 'fake_dbm')
    mclient.ping()
