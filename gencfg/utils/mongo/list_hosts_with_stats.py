#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import time
from argparse import ArgumentParser
import pymongo
import re

import gencfg
from gaux.aux_mongo import get_mongo_collection
from core.db import CURDB
from utils.pregen import get_extra_bot_machines
from utils.pregen import update_hosts
from gaux.aux_reserve import UNSORTED_GROUP


def parse_cmd():
    parser = ArgumentParser(description="List all hosts with stats in instancestate")
    parser.add_argument("-a", "--action", type=str, required=True,
                        choices=["show", "showunknown", "movetounsorted"],
                        help="Obligatory. Action to execute")
    parser.add_argument("-v", "--verbose", action="store_true", default=False,
                        help="Optional. Add verbose output")

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    options = parser.parse_args()

    return options


def main(options):
    mongocoll = get_mongo_collection('instanceusage')

    allhosts = list(mongocoll.find(
        {'ts': {'$gt': int(time.time()) - 60 * 60 * 2}},
        {'host': 1, '_id': 0},
    ))
    allhosts = list(set(map(lambda x: x['host'], allhosts)))

    if options.action in ["showunknown", "movetounsorted"]:
        allhosts = list(set(allhosts) - set(CURDB.hosts.get_host_names()))
    if options.action == "movetounsorted":
        for regexp, _ in get_extra_bot_machines.FILTERS:
            allhosts = filter(lambda x: not re.match(regexp, x), allhosts)
        allhosts = list(set(allhosts) & set(get_extra_bot_machines.get_bot_hosts()))

        if len(allhosts) > 0:
            update_hosts_options = {
                "action": "addfake",
                "hosts": allhosts,
                "verbose": options.verbose,
            }
            update_hosts.jsmain(update_hosts_options, False)

            unsorted_group = CURDB.groups.get_group(UNSORTED_GROUP)
            for hostname in allhosts:
                unsorted_group.addHost(CURDB.hosts.get_host_by_name(hostname))

            CURDB.update()

    return allhosts


def show_result(options, result):
    if options.action in ["show", "showunknown"]:
        print "Total %d hosts: %s" % (len(result), ",".join(result[:200]))
    elif options.action == "movetounsorted":
        print "Added to UNSORTED %d hosts from instancestate: %s" % (len(result), ",".join(result[:200]))
    else:
        raise Exception("Unknown action %s" % options.action)


if __name__ == '__main__':
    options = parse_cmd()
    result = main(options)
    show_result(options, result)
