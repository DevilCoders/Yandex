#!/skynet/python/bin/python
"""Script to manipulate with gencfg cached staff (RX-474)"""

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import time

import gencfg
from core.db import CURDB
from core.argparse.parser import ArgumentParserExt
from core.settings import SETTINGS
from core.card.types import Date
from gaux.aux_staff import list_staff_users, list_staff_groups


class EActions(object):
    """All action to perform with users"""
    SYNC = 'sync'
    ALL = [SYNC]



def get_parser():
    parser = ArgumentParserExt(description='Manipulate on with gencfg cached staff groups in various ways')
    parser.add_argument('-a', '--action', type=str, required=True,
                        choices=EActions.ALL,
                        help='Obligatory. Action to execute')
    parser.add_argument("-v", "--verbose", action="count", dest="verbose_level", default=0,
                        help="Optional. Verbose mode. Multiple -v options increase the verbosity. The maximum is 1.")

    return parser


def normalize(options):
    pass


def main_sync_staff_group(options):
    # remove old data
    CURDB.staffgroups.purge()

    # get new data
    for d in list_staff_groups():
        CURDB.staffgroups.add(d['name'], parent_group=d['parent_group'])

    CURDB.staffgroups.update()


def main_sync_staff_users(options):
    for d in list_staff_users():
        name = d['name']

        retired_at = d['retired_at']
        if retired_at is not None:
            ts = int(time.mktime(time.strptime(retired_at, '%Y-%m-%d')))
            retired_at = Date.create_from_timestamp(ts)

        if CURDB.staffgroups.has(d['staff_group']):
            staff_group = d['staff_group']
        else:
            staff_group = None

        if not CURDB.users.has_user(name):
            # add new user
            CURDB.users.add_user(name, staff_group=staff_group, retired_at=retired_at)
        else:
            user = CURDB.users.get_user(name)
            user.retired_at = retired_at
            user.staff_group = staff_group

    CURDB.users.update()


def check_removed_group_owners(group):
    owners = []
    for owner in group.card.owners:
        if CURDB.staffgroups.has(owner) or CURDB.users.has_user(owner) or owner.startswith('abc:'):
            owners.append(owner)
        else:
            print('Remove owner {} form {}'.format(owner, group.card.name))

    if owners != group.card.owners:
        group.card.owners = owners
        group.mark_as_modified()

    for slave_group in group.card.slaves:
        check_removed_group_owners(slave_group)


def main_check_removed_group_owners(options):
    for group in CURDB.groups.get_groups():
        check_removed_group_owners(group)
    CURDB.groups.update(smart=True)


def main_sync(options):
    # sync groups
    main_sync_staff_group(options)

    # sync users
    main_sync_staff_users(options)

    # sync gencfg groups owners
    main_check_removed_group_owners(options)


def main(options):
    if options.action == EActions.SYNC:
        main_sync(options)
    else:
        raise Exception('Unknown action {}'.format(options.action))


if __name__ == '__main__':
    options = get_parser().parse_cmd()

    normalize(options)

    result = main(options)
