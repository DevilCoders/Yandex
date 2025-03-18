#!/skynet/python/bin/python

# !/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

from argparse import ArgumentParser

import gencfg
from core.db import CURDB
from core.searcherlookup import Searcherlookup
import core.argparse.types as argparse_types


def parse_cmd():
    parser = ArgumentParser(description="Write searcherlookup")
    parser.add_argument("-o", "--output-file", dest="output_file", type=str, required=True,
                        help="Obligatory. Path to output file")
    parser.add_argument("-c", "--config", type=str, default=None,
                        help="Optional. Path to config")
    parser.add_argument("-g", "--groups", type=argparse_types.groups, default=None,
                        help="Optional. Generate searcherlookup only for specified groups")
    parser.add_argument("-f", "--filter", type = argparse_types.pythonlambda, default=lambda x: True,
                        help="Optional. Extra filter on groups processed")

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    options = parser.parse_args()

    return options


def main(options):
    sgen = Searcherlookup(CURDB, options.config)

    if options.groups is None:
        options.groups = [x for x in CURDB.groups.get_groups() if x.card.properties.created_from_portovm_group is None]

    options.groups = filter(options.filter, options.groups)

    for group in options.groups:
        print group
        sgen.append_slookup(group.generate_searcherlookup(normalize=False, add_guest_hosts=True))

    sgen.write_searcherlookup(options.output_file)


if __name__ == '__main__':
    options = parse_cmd()
    main(options)
