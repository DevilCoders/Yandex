# -*- encoding: utf-8 -*-
from __future__ import unicode_literals

import time

import pytest
from requests import exceptions

from blackboxer import utils


def test_requests_retry_session_failed():
    session = utils.requests_retry_session()

    t_start = time.time()

    with pytest.raises(exceptions.ConnectionError):
        session.get('http://localhost:9999')

    t_end = time.time()

    # backoff_factor = 0.3
    # retry = 1.8 = 0 + 0.6 + 1.2
    assert t_end - t_start > 1.8


@pytest.mark.skipif('True', 'httpbin not available from inner yandex network')
def test_requests_retry_session_failed_with_timeout():
    session = utils.requests_retry_session()

    t_start = time.time()

    with pytest.raises(exceptions.ConnectionError):
        session.get('http://httpbin.org/delay/10', timeout=1)

    t_end = time.time()

    # backoff_factor = 0.3, timeout = 1
    # retry = 4.8 = 1 + 0 + 1 + 0.6 + 1 + 1.2
    assert t_end - t_start > 4.8


@pytest.mark.skipif('True', 'httpbin not available from inner yandex network')
def test_requests_retry_session_failed_500():
    session = utils.requests_retry_session()

    t_start = time.time()

    with pytest.raises(exceptions.RetryError):
        session.get('http://httpbin.org/status/500')

    t_end = time.time()

    # backoff_factor = 0.3
    # retry = 1.8 = 0 + 0.6 + 1.2
    assert t_end - t_start > 1.8


def test_choose_first_not_none_dict():
    data = dict(first='1', second=None, third='1')
    assert 'first' in utils.choose_first_not_none(data) or 'third' in utils.choose_first_not_none(data)

    data = dict(first=None, second='1', third='1')
    assert 'second' in utils.choose_first_not_none(data) or 'third' in utils.choose_first_not_none(data)

    data = dict(first=None, second=None, third='1')
    assert 'third' in utils.choose_first_not_none(data)

    data = dict(first=None, second=None, third=None)
    assert utils.choose_first_not_none(data) is None


def test_choose_first_not_none_list():
    data = ['1', None, '2']
    assert utils.choose_first_not_none(data) == '1'

    data = [None, '1', '2']
    assert utils.choose_first_not_none(data) == '1'

    data = [None, None, '2']
    assert utils.choose_first_not_none(data) == '2'

    data = [None, None, None]
    assert utils.choose_first_not_none(data) is None
