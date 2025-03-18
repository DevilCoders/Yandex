#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

from argparse import ArgumentParser

import gencfg
from core.db import CURDB
from config import WEB_GENERATED_DIR
import core.argparse.types as argparse_types


def parse_cmd():
    parser = ArgumentParser(description="Remove generated int configs for groups from w-generated/all")
    parser.add_argument("-g", "--groups", type=argparse_types.groups, required=True,
                        help="Obligatory. List of groups to process")

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    options = parser.parse_args()

    return options


def main(options):
    all_intlookups = []
    for group in options.groups:
        if len(group.card.intlookups) == 0:
            raise Exception("Group %s does not have intlookups")
        all_intlookups.extend(map(lambda x: CURDB.intlookups.get_intlookup(x), group.card.intlookups))

    all_ints = sum(map(lambda x: x.get_int_instances(), all_intlookups), [])

    for intsearch in all_ints:
        fname = os.path.join(WEB_GENERATED_DIR, 'all',
                             '%s:%s.cfg' % (intsearch.host.name.partition('.')[0], intsearch.port))
        if not os.path.exists(fname):
            raise Exception("Not found int config %s" % fname)
        os.remove(fname)


if __name__ == '__main__':
    options = parse_cmd()
    main(options)
