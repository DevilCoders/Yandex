# encoding: utf-8
import os
import json
import traceback
import time
import struct

import requests
import yatest

from antiadblock.cryprox.cryprox.config.system import BYPASS_UIDS_TYPES, INTERNAL_EXPERIMENT_CONFIG


def wait_for_availability(url):
    retry(func=lambda: requests.get(url, timeout=2).status_code == 200,
          retry_count=5, timeout=2)
    return url


def retry(func, retry_count=10, timeout=2, exception_check=lambda e: True, timeout_multiplier=1):
    """
    :param func: function to be called and checked against
    :param retry_count: number of calls
    :param timeout: time to sleep between calls
    :param exception_check: function that is checked Exception returned by func, and repeat looping only if returns True
    :param timeout_multiplier: increase timeout on each step
    :return: same as func
    :raises: RetryError, Exception
    """

    last_exception = None
    for count in xrange(1, retry_count + 1):
        try:
            result = func()
            if not result:
                raise Exception('Check failed')
            return result
        except Exception as e:
            if not exception_check(e):
                raise
            last_exception = e
        time.sleep(timeout)
        timeout = timeout * timeout_multiplier
    raise RetryError('All calls to function ({}) have failed!{}'.format(
        func,
        ' Last exception was ({})'.format(traceback.format_exc()) if last_exception else ''))


class RetryError(StandardError):
    pass


def update_uids_file(uids):
    for device in BYPASS_UIDS_TYPES:
        bypass_uids_file = os.path.join(yatest.common.output_path("perm"), BYPASS_UIDS_TYPES[device])
        if os.path.exists(bypass_uids_file):
            os.rename(bypass_uids_file, bypass_uids_file + '.old')
        with open(bypass_uids_file, 'wb') as f:
            for uid in sorted(map(int, uids)):
                f.write(struct.pack("Q", uid))


def update_internal_experiment_config_file(config):
    filename = os.path.join(yatest.common.output_path("perm"), INTERNAL_EXPERIMENT_CONFIG)
    if os.path.exists(filename):
        os.rename(filename, filename + '.old')
    with open(filename, 'w') as f:
        json.dump(config, f)
