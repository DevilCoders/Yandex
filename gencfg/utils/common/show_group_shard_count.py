#!/skynet/python/bin/python
import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

from argparse import ArgumentParser

import gencfg
from core.db import CURDB

import core.argparse.types as argparse_types


def parse_cmd():
    parser = ArgumentParser(description="Script, total number of shards in group intlookups")
    parser.add_argument("-g", "--group", dest="group", type=argparse_types.group, required=True,
                        help="Obligatory. Group name")

    if len(sys.argv) == 1:
        sys.argv.append('-h')

    options = parser.parse_args()

    return options


def main(options):
    if len(options.group.card.intlookups) == 0:
        print "Group %s does not have intlookups" % options.group.card.name
    else:
        tiers = []
        shards = 0
        for intlookup in map(lambda x: CURDB.intlookups.get_intlookup(x), options.group.card.intlookups):
            tiers.extend(intlookup.tiers)
            shards += intlookup.get_shards_count()
        print "Group %s: %d shards; %s tiers" % (options.group.card.name, shards, ','.join(tiers))


if __name__ == '__main__':
    options = parse_cmd()
    main(options)
