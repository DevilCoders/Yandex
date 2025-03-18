import time
import logging
import traceback

import pytest
import requests


def pytest_addoption(parser):
    parser.addoption('--url', action='store', help='Base url for a live API instance', default=None)


@pytest.fixture(scope='session')
def live_app_url(request):
    url = request.config.getoption('--url')

    if url is None:
        raise pytest.skip("give --url parameter")

    wait_for_availability(url + '/ping')

    return url


def retry(func, retry_count=10, timeout=2, exception_check=lambda e: True, timeout_multiplier=1):
    """
    :param func: function to be called and checked against
    :param retry_count: number of calls
    :param timeout: time to sleep between calls
    :param exception_check: function that is checked Exception returned by func, and repeat looping only if returns True
    :param timeout_multiplier: increase timeout on each step
    :return: same as func
    :raises: Exception
    """

    last_exception = None
    for count in xrange(1, retry_count + 1):
        logging.debug('Calling {}. Call num = {}'.format(func, count))
        try:
            result = func()
            if not result:
                raise Exception('Check failed')
            return result
        except Exception as e:
            if not exception_check(e):
                raise
            logging.debug('Exception ({}) while calling ({}), retrying'.format(traceback.format_exc(e), func))
            last_exception = e
        time.sleep(timeout)
        timeout = timeout * timeout_multiplier
    raise Exception('All calls to function ({}) have failed!{}'.format(func,
                                                                       ' Last exception was ({})'.format(traceback.format_exc(last_exception))
                                                                       if last_exception else ''))


def wait_for_availability(url):
    # wait for two minutes
    # TODO: get rid of verify=False
    retry(func=lambda: requests.get(url, verify=False).status_code == 200, retry_count=60, timeout=2)
    return url
