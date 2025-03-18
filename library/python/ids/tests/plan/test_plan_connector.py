# encoding: utf-8
import responses
import pytest

from ids import exceptions
from ids.services.plan.connector import PlanConnector as Connector


def get_connector():
    return Connector('plan', 'http', user_agent='test', oauth_token='dummy')

@responses.activate
def test_auth_error():
    responses.add(
        method=responses.GET,
        url='http://plan/v2/services',
        status=403,
    )

    connector = get_connector()

    with pytest.raises(exceptions.AuthError):
        connector.get(resource='services')


@responses.activate
def test_internal_server_error():
    responses.add(
        method=responses.GET,
        url='http://plan/v2/services',
        status=500,
    )

    connector = get_connector()

    with pytest.raises(exceptions.BackendError):
        connector.get(resource='services')


@responses.activate
def test_internal_not_found():
    responses.add(
        method=responses.GET,
        url='http://plan/v2/services',
        status=500,
    )

    connector = get_connector()

    with pytest.raises(exceptions.IDSException):
        connector.get(resource='services')
