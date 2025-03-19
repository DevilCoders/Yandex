# -*- coding: utf-8 -*-
"""
State for waiting DNS availability. See MDB-8500
"""

import time
import logging

LOG = logging.getLogger(__name__)

def __virtual__():
    return 'dns_record'

def available(name, records):
    """
    Ensures that DNS is ready for service.
    Records - JSON list
    """
    not_resolved = check_dns_with_retry(records)
    ret = make_ret_info(name, not_resolved, records)
    return ret

def check_dns_with_retry(records, retries_max=20):
    not_resolved = True
    for retries in range(retries_max):
        not_resolved = \
            __salt__['dns_record.available'](records)
        if not_resolved:
            delay = get_delay_exp(retries)
            LOG.warn('Failed to resolve addresses {}, will retry within {} seconds'.format(not_resolved, delay))
            time.sleep(delay)
        else:
            break
    return not_resolved

def get_delay_exp(retries, max_delay=30):
    """
    Returns the next wait interval, in seconds,
    using an exponential backoff algorithm.
    """
    delay = 2 ** retries
    if delay > max_delay:
        delay = max_delay
    return delay

def make_ret_info(name, not_resolved, records):
    ret = {
        'name': name,
        'result': True,
        'comment': 'DNS is ready for service on nodes {}'.format(records),
        'changes': {}
    }
    if not_resolved:
        ret['result'] = False
        ret['comment'] = 'Can not resolve DNS records: {fqdns}'.format(fqdns=not_resolved)
    return ret
