#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

from argparse import ArgumentParser

import gencfg
from core.db import CURDB


def parse_cmd():
    parser = ArgumentParser(description="Dump all combination of golovan tags")
    parser.add_argument("-o", "--outfile", type=str, required=True,
                        help="Obligatory. Output file name")

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    options = parser.parse_args()

    return options


def main(options):
    result = set()
    for group in CURDB.groups.get_groups():
        if len(group.card.intlookups) == 0:
            if group.card.searcherlookup_postactions.custom_tier.enabled:
                tier = group.card.searcherlookup_postactions.custom_tier.tier_name
            else:
                tier = 'none'
            for instance in group.get_instances():
                for prj in group.card.tags.prj:
                    result.add((group.card.tags.ctype, group.card.tags.itype, prj, instance.host.location, tier))
        else:
            for intlookup in map(lambda x: CURDB.intlookups.get_intlookup(x), group.card.intlookups):
                for instances, tier in intlookup.get_used_instances_with_tier():
                    for instance in instances:
                        for prj in group.card.tags.prj:
                            result.add(
                                (group.card.tags.ctype, group.card.tags.itype, prj, instance.host.location, tier))

    with open(options.outfile, "w") as f:
        for elem in sorted(result):
            print >> f, "a_ctype_%s,a_itype_%s,a_prj_%s,a_geo_%s,a_tier_%s" % elem


if __name__ == '__main__':
    options = parse_cmd()
    main(options)
