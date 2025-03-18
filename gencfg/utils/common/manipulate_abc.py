#!/skynet/python/bin/python
"""Script to manipulate with gencfg cached abc groups (RX-447)"""

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import json

import gencfg
from core.db import CURDB
from core.argparse.parser import ArgumentParserExt
from core.settings import SETTINGS
from gaux.aux_abc import list_abc_services, list_abc_services_members
from gaux.aux_colortext import red_text


class EActions(object):
    """All action to perform with abc cache"""
    SYNC = 'sync'
    ALL = [SYNC]


def get_parser():
    parser = ArgumentParserExt(description='Manipulate with cached abc groups in various ways')
    parser.add_argument('-a', '--action', type=str, required=True,
                        choices=EActions.ALL,
                        help='Obligatory. Action to execute')
    parser.add_argument("-v", "--verbose", action="count", dest="verbose_level", default=0,
                        help="Optional. Verbose mode. Multiple -v options increase the verbosity. The maximum is 1.")

    return parser


def normalize(options):
    pass


def rename_abc_in_group(group, old_abc_name, new_abc_name):
    new_owners = []
    for owner in group.card.owners:
        if owner == 'abc:{}'.format(old_abc_name):
            owner = 'abc:{}'.format(new_abc_name)
        if owner.startswith('abc:{}:'.format(old_abc_name)):
            owner = 'abc:{}:{}'.format(new_abc_name, owner.partition(':')[2].partition(':')[2])
        new_owners.append(owner)

    if new_owners != group.card.owners:
        group.card.owners = new_owners
        group.mark_as_modified()


def main_sync(options):
    old_abc_id_name_mapping = {x['id']: x['slug'] for x in CURDB.abcgroups.abc_services.values()}

    CURDB.abcgroups.purge()

    # add services
    remote_services = list_abc_services()
    for elem in remote_services:
        # print json.dumps(elem, indent=4)
        if elem['parent'] is not None:
            parent_service_id = elem['parent']['id']
        else:
            parent_service_id = None
        CURDB.abcgroups.add_service(elem['slug'].lower(), elem['id'], parent_service_id)

    # fill roles
    remote_service_owners = list_abc_services_members()
    for elem in remote_service_owners:
        # print json.dumps(elem, indent=4)
        scope_slug = elem['role']['scope']['slug']
        scope_id = elem['role']['scope']['id']

        if not CURDB.abcgroups.has_scope(scope_slug, scope_id=scope_id):
            CURDB.abcgroups.add_scope(scope_slug, scope_id)

        user = elem['person']['login']
        service_slug = elem['service']['slug'].lower()

        if not CURDB.abcgroups.has_service(service_slug):
            print red_text('Service <{}> not found in database'.format(service_slug))
        else:
            CURDB.abcgroups.add_member_to_service_scope(service_slug, scope_slug, user)

    CURDB.abcgroups.mark_as_modified()

    # rename abc groups
    new_abc_id_name_mapping = {x['id']: x['slug'] for x in CURDB.abcgroups.abc_services.values()}
    renames = []
    for abc_id in new_abc_id_name_mapping:
        if abc_id not in old_abc_id_name_mapping:
            continue
        if new_abc_id_name_mapping[abc_id] == old_abc_id_name_mapping[abc_id]:
            continue

        new_abc_name, old_abc_name = new_abc_id_name_mapping[abc_id], old_abc_id_name_mapping[abc_id]

        # replace abc:oldname to abc:newname in all groups
        for group in CURDB.groups.get_groups():
            rename_abc_in_group(group, old_abc_name, new_abc_name)

    CURDB.update(smart=True)


def main(options):
    if options.action == EActions.SYNC:
        main_sync(options)
    else:
        raise Exception('Unknown action {}'.format(options.action))


def print_result(result, options):
    print result


if __name__ == '__main__':
    options = get_parser().parse_cmd()

    normalize(options)

    result = main(options)

    print_result(result, options)
