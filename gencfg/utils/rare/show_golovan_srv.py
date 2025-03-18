#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))

from argparse import ArgumentParser

import gencfg
from core.db import CURDB
import core.argparse.types as argparse_types


def parse_cmd():
    parser = ArgumentParser(description="Show first and second replica for new golvoan srv")
    parser.add_argument("-g", "--group", type=argparse_types.group, required=True,
                        help="Obligatory. Name of golovan main group")

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    options = parser.parse_args()

    return options


def main(options):
    for slave in options.group.slaves:
        if len(slave.intlookups) == 0:
            continue

        kvstore = dict(map(lambda x: (x.name, x.value), slave.properties.kvstore))
        intlookup = CURDB.intlookups.get_intlookup(slave.intlookups[0])

        first_replica = sum(map(lambda x: x.brigades[0].get_all_basesearchers(), intlookup.brigade_groups), [])
        second_replica = sum(map(lambda x: x.brigades[1].get_all_basesearchers(), intlookup.brigade_groups), [])

        print "Slave group %s (queue %s):" % (slave.name, kvstore['served_queue'])
        print "    First replica (queue %s): %s" % (
        first_replica[0].host.queue, ','.join(map(lambda x: x.host.name, first_replica)))
        print "    Second replica (queue %s): %s" % (
        second_replica[0].host.queue, ','.join(map(lambda x: x.host.name, second_replica)))


if __name__ == '__main__':
    options = parse_cmd()
    main(options)
