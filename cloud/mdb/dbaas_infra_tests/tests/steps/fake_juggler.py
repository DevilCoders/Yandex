"""
Steps related to Fake Juggler.
"""

from behave import when

from tests.helpers import fake_juggler, mock
from tests.helpers.step_helpers import step_require
from tests.steps import internal_api as intapi_steps


@when('we reset all the juggler crits')
def step_reset_crits(context):
    mclient = mock.MockClient(context, 'fake_juggler')
    mclient.reset()


@when('"{host_type:w}" host in "{geo}" has CRIT for "{service}"')
@step_require('hosts')
def step_set_crit(context, host_type, geo, service):
    host = intapi_steps.find_host(context, host_type, geo)
    fake_juggler.set_failed(context, host, service)
