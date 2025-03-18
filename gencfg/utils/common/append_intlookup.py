#!/skynet/python/bin/python
import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

from argparse import ArgumentParser

import gencfg
from core.db import CURDB
import core.argparse.types as argparse_types

JOIN_TYPES = ['shards', 'replicas']


def parse_cmd():
    parser = ArgumentParser("Join 2 intlookups together")
    parser.add_argument("-j", "--join-type", type=str, required=True,
                        choices=JOIN_TYPES,
                        help="Obligatory. Join type: %s" % ','.join(JOIN_TYPES))
    parser.add_argument("-s", "--src-intlookup", type=argparse_types.intlookup, required=True,
                        help="Obligatory. Src intlookup (will be added to dst intlookup)")
    parser.add_argument("-d", "--dst-intlookup", type=argparse_types.intlookup, required=True,
                        help="Obligatory. Dst intlookup")
    parser.add_argument("--remove-src-intlookup", action="store_true", default=False,
                        help="Optional. Remove src intlookup after append")

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    options = parser.parse_args()

    return options


def main(options):
    if options.join_type == 'replicas':
        check_equal = ['brigade_groups_count', 'hosts_per_group', 'tiers']
    elif options.join_type == 'sharts':
        check_equal = ['hosts_per_group']
    else:
        check_equal = []
#    for param in check_equal:
#        if options.src_intlookup.__dict__.get(param) != options.dst_intlookup.__dict__.get(param):
#            raise Exception("Param %s in intlookups %s and %s is different" % (param, options.src_intlookup.file_name, options.dst_intlookup.file_name))

    if options.join_type == 'replicas':
        src_multishards = options.src_intlookup.get_multishards()
        dst_multishards = options.dst_intlookup.get_multishards()

        assert len(src_multishards) == len(dst_multishards)
        for src_multishard, dst_multishard in zip(src_multishards, dst_multishards):
            dst_multishard.brigades.extend(src_multishard.brigades)
            src_multishard.brigades = []
    elif options.join_type == 'shards':
        options.dst_intlookup.brigade_groups.extend(options.src_intlookup.brigade_groups)
        options.dst_intlookup.brigade_groups_count += options.src_intlookup.brigade_groups_count
        if options.dst_intlookup.tiers is not None and options.src_intlookup.tiers is not None:
            options.dst_intlookup.tiers.extend(options.src_intlookup.tiers)
    else:
        raise Exception("Unknown join_type %s" % options.join_type)

    if options.remove_src_intlookup:
        CURDB.intlookups.remove_intlookup(options.src_intlookup.file_name)

    options.dst_intlookup.mark_as_modified()

    CURDB.intlookups.update(smart=True)


if __name__ == '__main__':
    options = parse_cmd()
    main(options)
