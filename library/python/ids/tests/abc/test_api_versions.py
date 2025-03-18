# coding: utf-8
from __future__ import unicode_literals

from ids.registry import registry


def test_api_version_4():
    repo = registry.get_repository(
        service='abc',
        resource_type='service',
        user_agent='ids-test',
        service_ticket='service_ticket',
        api_version=4,
    )
    actual_patterns = repo.connector.url_patterns
    expected_patterns = {
        'service': '/api/v4/services/',
        'service_contacts': '/api/v4/services/contacts/',
        'service_members': '/api/v4/services/members/',
        'role': '/api/v4/roles/',
        'resource': '/api/v4/resources/',
    }
    assert actual_patterns == expected_patterns


def test_default_api_version():
    repo = registry.get_repository(
        service='abc',
        resource_type='service',
        user_agent='ids-test',
        service_ticket='service_ticket',
    )
    actual_patterns = repo.connector.url_patterns
    expected_patterns = {
        'service': '/api/v3/services/',
        'service_contacts': '/api/v3/services/contacts/',
        'service_members': '/api/v4/services/members/',
        'role': '/api/v3/roles/',
        'resource': '/api/v3/resources/',
    }
    assert actual_patterns == expected_patterns
