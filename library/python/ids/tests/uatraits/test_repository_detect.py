# -*- coding: utf-8 -*-
from __future__ import unicode_literals

import pytest

from ids.registry import registry


@pytest.fixture
def detect():
    return registry.get_repository('uatraits', 'detect', user_agent='test')


@pytest.mark.integration
def test_detect_data_ok(detect):
    data = '{"User-Agent":"Mozilla/5.0 (Windows NT 6.1; WOW64; rv:40.0) Gecko/20100101 Firefox/40.1"}'
    result = detect.get_one({'data': data})
    assert 'Gecko' == result['BrowserEngine']
    assert 'Firefox' == result['BrowserName']
    assert 'Windows' == result['OSFamily']
    assert result['isBrowser']
    assert not result['isMobile']


@pytest.mark.integration
def test_detect_user_agent_ok(detect):
    user_agent = 'Mozilla/5.0 (Windows NT 6.1; WOW64; rv:40.0) Gecko/20100101 Firefox/40.1'
    result = detect.get_one({'user_agent': user_agent})
    assert 'Gecko' == result['BrowserEngine']
    assert 'Firefox' == result['BrowserName']
    assert 'Windows' == result['OSFamily']
    assert result['isBrowser']
    assert not result['isMobile']


@pytest.mark.integration
def test_detect_data_error(detect):
    result = detect.get_one({'data': ''})
    assert 'Unknown' == result['BrowserEngine']
    assert 'Unknown' == result['BrowserName']
    assert 'Unknown' == result['OSFamily']
    assert not result['isBrowser']
    assert not result['isMobile']


@pytest.mark.integration
def test_detect_user_agent_error(detect):
    result = detect.get_one({'user_agent': ''})
    assert 'Unknown' == result['BrowserEngine']
    assert 'Unknown' == result['BrowserName']
    assert 'Unknown' == result['OSFamily']
    assert not result['isBrowser']
    assert not result['isMobile']
