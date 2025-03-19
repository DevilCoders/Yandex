# -*- coding: utf-8 -*-
"""
Module to check DNS availability
:platform: all
"""

from __future__ import absolute_import

import logging

log = logging.getLogger(__name__)

try:
    import socket

    MODULES_OK = True
except ImportError:
    MODULES_OK = False

__virtualname__ = 'dns_record'


def __virtual__():
    """
    Determine whether or not to load this module
    """
    if MODULES_OK:
        return __virtualname__
    return False


def available(records):
    """
    Method for check DNS answer.
    Input:
        records -- list of fqdns
    Output:
        not_resolved -- list of not resolved fqdns
    """
    not_resolved = []
    for record in records:
        try:
            socket.getaddrinfo(record, 0)
        except (socket.gaierror, socket.timeout):
            not_resolved.append(record)
    return not_resolved
