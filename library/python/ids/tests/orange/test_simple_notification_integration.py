# coding: utf-8
from __future__ import unicode_literals

import pytest

from ids.services.orange import api


germes_uid = 1120000000015421


@pytest.fixture
def oauth_token():
    # from ids.helpers import oauth
    # uid = '1120000000015421'
    # oauth.get_token(uid=uid, oauth_id=OAUTH_ID, oauth_secret=OAUTH_SECRET)
    # OAUTH_ID должно быть зарегистрировано в orange

    oauth_token = 'f1fcdc4c02db442a9e8a9cb89d81aa0e'  # zomb-prj-282' by planner
    return oauth_token


@pytest.mark.integration
def test_ping(oauth_token):
    response_data = api.ping(oauth_token=oauth_token, user_agent='ids-test').json()

    assert response_data['result'] == 'success'


@pytest.mark.integration
def test_simple_notification(oauth_token):
    response = api.push_notification(
        oauth_token=oauth_token,
        user_agent='ids-test',
        description={'ru': {'header': 'Hello, cruel world'}},
        target_uids=[germes_uid],
    )
    response_data = response.json()

    assert 'notificationId' in response_data

    notification_id = response_data['notificationId']
    _clean(oauth_token=oauth_token, notification_id=notification_id)


@pytest.mark.integration
def test_simple_callback_notification(oauth_token):
    response = api.push_callback_notification(
        oauth_token=oauth_token,
        user_agent='ids-test',
        description={'ru': {
            'header': 'The world wants to become more cruel. Do you accept?'
        }},
        target_uids=[germes_uid],
        request_type='service_move_approve',
        accept_type='ANY',
    )
    response_data = response.json()

    assert 'notificationId' in response_data

    notification_id = response_data['notificationId']
    _clean(oauth_token=oauth_token, notification_id=notification_id)


def _clean(oauth_token, notification_id):
    response = api.delete_notification(
        oauth_token=oauth_token,
        user_agent='ids-test',
        id=notification_id,
    )

    assert response.status_code == 204

