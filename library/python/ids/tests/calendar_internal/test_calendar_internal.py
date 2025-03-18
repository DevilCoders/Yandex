# coding: utf-8
from __future__ import unicode_literals

import pytest

from mock import patch, ANY

from ids.registry import registry
from ids.resource import Resource

from ..helpers import FakeResponse


@patch('requests.Session.request', return_value=FakeResponse(200))
def test_update_event(mocked_request):
    repo = registry.get_repository(
        service='calendar_internal',
        resource_type='event',
        user_agent='femida',
        service_ticket='service_ticket',
        user_ticket='user_ticket',
    )
    resource = Resource({'id': 1, 'uid': 2})
    data = {
        'name': 'name',
        'description': 'description',
        'startTs': '2020-10-02T20:00:00',
        'endTs': '2020-10-02T21:00:00',
    }

    repo.update(resource=resource, fields=data)

    mocked_request.assert_called_once_with(
        'POST',
        'https://calendar-api.testing.yandex-team.ru/internal/update-event',
        params=resource,
        json=data,
        timeout=ANY,
        headers=ANY,
    )
