# -*- coding: utf-8 -*-
"""
Simple resolv.conf parse helper
"""

import os

__salt__ = {}


def __virtual__():
    """
    We always return True here (we are always available)
    """
    return True


def get_compute_dns():
    """
    Parse resolv.conf to get compute dns address
    """
    if os.name == 'nt' or __salt__['pillar.get']('data:dbaas:vtype', 'unknown') != 'compute':
        return None
    with open('/etc/resolv.conf') as resolv:
        selected = None
        for line in resolv:
            if line.startswith('nameserver'):
                splitted = line.split()
                if len(splitted) < 2:
                    continue
                address = splitted[1]
                if address not in ('127.0.0.1', '::1'):
                    selected = address
        return selected
