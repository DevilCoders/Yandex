# coding: utf-8
from __future__ import unicode_literals

import mock

from django_pgaas.utils import RandomisedBackoffRange, logged_retry_range

MAX_RETRIES = 10
RETRY_SLOT = 7
LABEL = 'Foooo Baaar'


def right(a, b):
    return b


def test_randomised_backoff_range():

    with mock.patch('time.sleep') as sleep_mocked, \
            mock.patch('random.randint', new=right) as randint_mocked:

        retry_range = RandomisedBackoffRange(MAX_RETRIES, RETRY_SLOT)

        attempts = list(retry_range)

        assert len(attempts) == MAX_RETRIES + 1
        assert sleep_mocked.call_count == MAX_RETRIES

        prev = 0

        for kall in sleep_mocked.call_args_list:
            args, _ = kall
            # With mocked randint() always returning its right boundary,
            # sleep() must be called with an increasing sequence
            # of multiples of RETRY_SLOT.
            assert args[0] > 0
            assert not args[0] % RETRY_SLOT
            assert prev < args[0]
            prev = args[0]


@mock.patch('time.sleep')
def test_logged_range_seq(sleep_mocked):

    backoff_range = RandomisedBackoffRange(MAX_RETRIES, RETRY_SLOT)

    retry_range = logged_retry_range(LABEL, backoff_range)

    seq_logged = list(retry_range)
    assert sleep_mocked.call_count == MAX_RETRIES

    seq_not_logged = list(backoff_range)

    assert seq_logged == seq_not_logged


@mock.patch('time.sleep')
def test_logged_range_logging(sleep_mocked):

    backoff_range = RandomisedBackoffRange(MAX_RETRIES, RETRY_SLOT)

    with mock.patch('django_pgaas.utils.logger') as logger_mocked:
        retry_range = logged_retry_range(LABEL, backoff_range)

        for _ in retry_range:
            pass

        assert logger_mocked.exception.call_count == 1
        assert logger_mocked.warning.call_count == MAX_RETRIES
