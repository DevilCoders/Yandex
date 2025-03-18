#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

from argparse import ArgumentParser
import random

import gencfg
from core.db import CURDB

LOCATIONS = ["MSK", "SAS", "MAN", "VLA"]

ACTIONS = ["prepare_trunk", "prepare_branch", "prepare_merge"]


def parse_cmd():
    parser = ArgumentParser(description="Prepare reserved groups for recluster")
    parser.add_argument("-a", "--action", type=str, required=True,
                        choices=ACTIONS,
                        help="Obligatory. Action to execute")
    parser.add_argument("-k", "--keep", type=int, default=100,
                        help="Optional. Number of hosts to keep in reserved in every DC (prepare_trunk action)")

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    options = parser.parse_args()

    return options


def main(options):
    curtag = CURDB.get_repo().get_current_tag()
    if curtag is not None:
        raise Exception("Trying to work with tag <%s> (only trunk or branches are allowed" % curtag)

    curbranch = CURDB.get_repo().get_current_branch()
    if options.action == "prepare_trunk":
        if curbranch is not None:
            raise Exception("Trying to perform action <%s> on branch <%s>" % (options.action, curbranch))

        for location in LOCATIONS:
            group = CURDB.groups.get_group("%s_RESERVED" % location)
            oldgroup = CURDB.groups.get_group("%s_RESERVED_OLD" % location)

            if len(oldgroup.getHosts()) > 0:
                raise Exception("Group %s not empty" % oldgroup.card.name)

            hosts = group.getHosts()
            random.shuffle(hosts)
            move_hosts = hosts[options.keep:]

            CURDB.groups.move_hosts(move_hosts, oldgroup)
    elif options.action == "prepare_branch":
        if not curbranch.startswith("pre-stable-"):
            raise Exception("Trying to perform action <%s> on branch <%s>" % (options.action, curbranch))

        for location in LOCATIONS:
            group = CURDB.groups.get_group("%s_RESERVED" % location)
            group_hosts = group.getHosts()

            oldgroup = CURDB.groups.get_group("%s_RESERVED_OLD" % location)
            oldgroup_hosts = oldgroup.getHosts()

            CURDB.groups.move_hosts(group_hosts, oldgroup)
            CURDB.groups.move_hosts(oldgroup_hosts, group)
    elif options.action == "prepare_merge":
        # if not curbranch.startswith("pre-stable-"):
        #     raise Exception("Trying to perform action <%s> on branch <%s>" % (options.action, curbranch))

        for location in LOCATIONS:
            group = CURDB.groups.get_group("%s_RESERVED" % location)
            oldgroup = CURDB.groups.get_group("%s_RESERVED_OLD" % location)

            oldgroup_hosts = oldgroup.getHosts()

            CURDB.groups.move_hosts(oldgroup_hosts, group)
    else:
        raise Exception("Unknown action %s" % options.action)

    CURDB.update(smart=True)


if __name__ == '__main__':
    options = parse_cmd()
    main(options)
