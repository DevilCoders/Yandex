#!/usr/bin/env python
# -*- coding: utf-8 -*-
""" The utility to be run. The world to be captured. """
from __future__ import print_function, absolute_import

import json

import helpers
from helpers import CONSOLE as log


@helpers.handle_error('Failed! AutoReconnect limit reached',
                      3, 1, 'AutoReconnect')
def add_user(client, data, dry_run=True):
    """ Adds a user to the database """
    log.trace("New users data: {0}".format(json.dumps(data, indent=2, sort_keys=True, default=str)))
    if data['roles']:
        log.info("Add/Update user: '{0}'".format(data['name']))
        for db, roles in data['roles'].items():         # pylint: disable=C0103
            if not roles:
                msg = "User '{0[name]}' without roles in db '{1}'. Skip!"
                log.warn(msg.format(data, db))
                continue
            msg = "Add roles for '{0[name]}' in db '{2}': {1} on host {3}"
            log.debug(msg.format(data, json.dumps(roles, indent=2, sort_keys=True, default=str), db, client))
            if not dry_run:
                client[db].add_user(data['name'], data['passwd'], roles=roles)
    else:
        log.trace("User {0} without roles, skip!".format(data))


@helpers.handle_error('Failed! AutoReconnect limit reached',
                      3, 1, 'AutoReconnect')
def cleanup_users(client, users, clean=False, dry_run=True):
    """ Removes users from the database """
    dbs = client.admin.command('listDatabases')['databases']
    dbs = set(x['name'] for x in dbs)
    dbs = dbs - {'config', 'local'}

    for db in dbs:                                      # pylint: disable=C0103
        db_users = client[db].command({'usersInfo': 1})
        log.trace("Database users: {0}".format(json.dumps(db_users, indent=2, sort_keys=True, default=str)))

        db_users = db_users['users']
        db_users = set(x['user'] for x in db_users)

        if clean:  # clean grantless users
            for user in users.keys():
                if not any(users[user]['roles'].values()):
                    log.debug("Clean grantless user '{0}'.".format(users[user]))
                    del users[user]
        not_empty = set([u for u in users])

        del_this_users = db_users - not_empty
        for user in del_this_users:
            log.info("Remove stale user '{0}' from db '{1}'".format(user, db))
            if not dry_run:
                client[db].remove_user(user)

    return users
