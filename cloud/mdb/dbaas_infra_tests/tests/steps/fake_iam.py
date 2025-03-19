"""
Steps related to fake iam and access service
"""

import requests
from behave import given, when
from retrying import retry

from tests.helpers.fake_iam import (get_access_service_url, get_identity_url, get_resource_manager_control_url)
from tests.helpers.side_effects import side_effects


@given('disabled authentication in access service')
@side_effects(scope='scenario', projects=['fake_iam'])
@retry(wait_fixed=1, stop_max_attempt_number=10)
def step_disable_authorization(context):
    url = get_access_service_url(context)
    requests.put('{url}/authenticate'.format(url=url)).raise_for_status()


@given('disabled authorization in access service')
@side_effects(scope='scenario', projects=['fake_iam'])
@retry(wait_fixed=1, stop_max_attempt_number=10)
def step_disable_authentication(context):
    url = get_access_service_url(context)
    requests.put('{url}/authorize'.format(url=url)).raise_for_status()


@given('feature flag "{flag_name:w}" is set')
@when('we enable feature flag "{flag_name:w}"')
@retry(wait_fixed=1000, stop_max_attempt_number=10)
def set_feature_flag(context, flag_name):
    """
    Set feature flag with fake iam
    """
    iamurl = get_identity_url(context)
    requests.post(
        '{url}/v1/clouds/rmi99999999999999999:addPermissionStages'.format(url=iamurl),
        json={
            'permissionStages': [flag_name],
        },
    ).raise_for_status()

    rmurl = get_resource_manager_control_url(context)
    requests.post(
        '{url}/v1/clouds/rmi99999999999999999:addPermissionStages'.format(url=rmurl),
        json={
            'permissionStages': [flag_name],
        },
    ).raise_for_status()
