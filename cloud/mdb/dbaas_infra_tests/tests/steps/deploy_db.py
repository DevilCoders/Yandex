"""
Steps related to DeployDB
"""

from behave import given

from tests.helpers.deploy_db import get_latest_version, get_version
from tests.helpers.workarounds import retry


@given('actual DeployDB version')
@retry(wait_fixed=1000, stop_max_attempt_number=300)
def wait_for_deploydb(context):
    """
    Wait until MDB Deploy DB has latest migrations version
    """
    current = get_version(context)
    latest = get_latest_version(context)
    msg = 'DeployDB has version {current}, latest {latest}'.format(
        current=current,
        latest=latest,
    )
    assert current == latest, msg
