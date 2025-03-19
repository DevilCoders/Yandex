# -*- coding: utf-8 -*-
"""
MySQL utility functions for MDB
"""

from __future__ import absolute_import, print_function, unicode_literals

import copy
import logging

log = logging.getLogger(__name__)

SYSTEM_USER_NAMES = ['admin', 'repl', 'monitor']

SYSTEM_IDM_USER_NAMES = ['mdb_writer', 'mdb_reader']

__salt__ = {}
__pillar__ = None


def __virtual__():
    """
    We always return True here (we are always available)
    """
    return True


def get_users(filter=None):
    users = {}
    system_users = {}
    idm_system_users = {}
    idm_users = {}
    all_users = copy.copy(__salt__['pillar.get']('data:mysql:users', {}))
    for user_name, u in all_users.items():
        if filter and filter != user_name:
            continue
        if user_name in SYSTEM_USER_NAMES:
            system_users[user_name] = u
        elif user_name in SYSTEM_IDM_USER_NAMES:
            idm_system_users[user_name] = u
        elif u.get('origin') == 'idm':
            idm_users[user_name] = u
        else:
            users[user_name] = u

    return users, system_users, idm_system_users, idm_users
