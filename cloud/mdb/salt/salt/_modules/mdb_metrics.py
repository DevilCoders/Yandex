# -*- coding: utf-8 -*-
"""
mdb-metrics module for salt
"""

from __future__ import absolute_import, print_function, unicode_literals

import datetime
import logging
import os
import re

__salt__ = {}


LOG = logging.getLogger(__name__)
_default = object()


def __virtual__():
    """
    We always return True here (we are always available)
    """
    return True


def pillar(key, default=_default):
    """
    Like __salt__['pillar.get'], but when the key is missing and no default value provided,
    a KeyError exception will be raised regardless of 'pillar_raise_on_missing' option.
    """
    value = __salt__['pillar.get'](key, default=default)
    if value is _default:
        raise KeyError('Pillar key not found: {0}'.format(key))

    return value


def get_sender_template():
    return pillar('data:monrun:mdb-metrics:sender_template', '/tmp/mdb-metrics-{}-sender.state')


def get_sender_warn_limit():
    return pillar('data:monrun:mdb-metrics:sender_warn', '300')


def get_sender_crit_limit():
    return pillar('data:monrun:mdb-metrics:sender_crit', '600')


def get_redis_no_slots_marker_filename():
    return '/var/tmp/mdb-redis.sharded.shard_with_no_slots'


def get_redis_oom_command_state_file():
    return '/tmp/oom_command_state_file'


def get_redis_shared_metrics():
    return '/home/monitor/info.json'
