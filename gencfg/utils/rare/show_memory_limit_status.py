#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import gencfg
from core.db import CURDB
from core.argparse.parser import ArgumentParserExt
import core.argparse.types


def get_parser():
    parser = ArgumentParserExt(description="Show memory limits status")
    parser.add_argument("-g", "--groups", type=core.argparse.types.xgroups, required=True,
                        help="Obligatory. Comma-separated list of igroups regexp (like SAS_.*,.*_BASE,...)")
    parser.add_argument("-f", "--flt", type=core.argparse.types.pythonlambda, default=lambda x: True,
                        help="Optional. Filter on groups")
    parser.add_argument("-v", "--verbose", action="count", default=0,
                        help="Optional. Increase output verbosity (maximum 1)")

    return parser


def show_groups(groups):
    for group in sorted(groups, cmp=lambda x, y: cmp(x.card.name, y.card.name)):
        print "        Group %s (master %s)" % (
        group.card.name, None if group.card.master is None else group.card.master.card.name)


def main(options):
    SUBS = [
        ("groups with limit set", lambda x: x.card.reqs.instances.memory_guarantee.value > 0),
        ("fake groups", lambda x: x.card.properties.fake_group == True),
        ("nonsearch groups", lambda x: x.card.properties.nonsearch == True),
        ("full host groups", lambda x: x.card.properties.full_host_group == True),
    ]

    groups = filter(options.flt, options.groups)
    print "Found %d groups" % (len(groups))

    processed_groups = set()
    for descr, flt in SUBS:
        filtered = set(filter(flt, groups)) - processed_groups
        processed_groups |= filtered
        print "    Found %s %s" % (len(filtered), descr)
        if options.verbose >= 1:
            show_groups(filtered)
    nodata_groups = set(groups) - set(processed_groups)
    print "    Found %d groups without limits set" % len(nodata_groups)
    if options.verbose >= 1:
        show_groups(nodata_groups)


if __name__ == '__main__':
    options = get_parser().parse_cmd()
    main(options)
