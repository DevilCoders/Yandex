# coding: utf-8

from __future__ import unicode_literals

import json

import pytest
from pretend import stub

from ids.exceptions import BackendError
from ids.services.startrek2.connector import StConnector


@pytest.fixture
def connector():
    return StConnector(user_agent='ids-test', oauth_token='dummy')


def assert_expected_exception(excinfo, message, status_code):
    exc = excinfo.value
    assert exc.status_code == status_code
    assert str(exc) == message


def test_handle_bad_response_really_bad(connector):
    response = stub(
        status_code=500,
        text='omg wtf',
        reason='Server Error',
    )
    with pytest.raises(BackendError) as excinfo:
        connector.handle_bad_response(response)

    assert_expected_exception(
        excinfo=excinfo,
        message='Backend responded with 500 (Server Error)',
        status_code=500,
    )


def test_handle_bad_response_expected(connector):
    response = stub(
        status_code=400,
        text=json.dumps({
            'errors': {},
            'errorMessages': [u'Your json suck']
        }),
        reason='Bad Request',
    )
    with pytest.raises(BackendError) as excinfo:
        connector.handle_bad_response(response)

    assert_expected_exception(
        excinfo=excinfo,
        message='Invalid JSON data was sent. Your json suck',
        status_code=400,
    )


def test_handle_bad_response_default_msg(connector):
    response = stub(
        status_code=422,
        text=json.dumps({
            'errors': {},
            'errorMessages': [
                'Users [vasya, petya, kolya] do not exist',
                'And you suck',
            ]
        }),
        reason='Unprocessable Entity',
    )
    with pytest.raises(BackendError) as excinfo:
        connector.handle_bad_response(response)

    assert_expected_exception(
        excinfo=excinfo,
        message='Backend responded with 422 (Unprocessable Entity). ' +
                'Users [vasya, petya, kolya] do not exist, ' +
                'And you suck',
        status_code=422,
    )
