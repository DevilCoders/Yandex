#!/skynet/python/bin/python
"""
    We do not want single unsorted group with "everything left". Looks like we better split big ALL_UNSORTED group into a lot of smaller group.
    Instances in this groups will be grouped by bot prj.
"""

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

from collections import defaultdict
import re

import gencfg
from core.db import CURDB
from core.argparse.parser import ArgumentParserExt
from gaux.aux_colortext import red_text
from gaux.aux_reserve import UNSORTED_GROUP, PRENALIVKA_GROUP, ALL_PRENALIVKA_TAGS
import gaux.aux_decorators


def get_parser():
    parser = ArgumentParserExt("Script to update unsorted groups")
    parser.add_argument("-l", "--low-limit", type=int, required=True,
                        help="Obligatory. If we have more than <low_limit> machines with specific botprj, we automatically create unsorted group")
    parser.add_argument("-y", "--apply", dest="apply", action="store_true", default=False,
                        help="Optional. Apply changes (otherwise db will not be changed)")
    parser.add_argument("-v", "--verbose", action="count", dest="verbose_level", default=0,
                        help="Optional. Verbose mode. Multiple -v options increase the verbosity.  The maximum is 1.")

    return parser


@gaux.aux_decorators.memoize
def _convert_host_to_group_name(host):
    if ALL_PRENALIVKA_TAGS & set(host.walle_tags):
        return PRENALIVKA_GROUP

    botprj = host.botprj

    postfix = re.sub("[^A-Z0-9]+", "_", str.upper(botprj)).rstrip("_")

    if postfix == "":
        return UNSORTED_GROUP
    return "ALL_UNSORTED_%s" % postfix


def main(options):
    unsorted_groups = filter(lambda x: x.card.tags.metaprj == 'unsorted' and x.card.master is None, CURDB.groups.get_groups())

    mapping = defaultdict(list)
    mapping[UNSORTED_GROUP] = []
    for group in unsorted_groups:
        group.mark_as_modified()
        for host in group.getHosts():
            mapping[_convert_host_to_group_name(host)].append(host)

    for k in mapping:
        if k in [UNSORTED_GROUP, PRENALIVKA_GROUP] or len(mapping[k]) >= options.low_limit:
            continue
        mapping[UNSORTED_GROUP].extend(mapping[k])
    mapping = {k: v for k, v in mapping.iteritems() if
               k in [UNSORTED_GROUP, PRENALIVKA_GROUP] or len(v) >= options.low_limit}

    for groupname in mapping:
        if not CURDB.groups.has_group(groupname):
            CURDB.groups.copy_group(UNSORTED_GROUP, groupname)

        group = CURDB.groups.get_group(groupname)

        to_add = set(mapping[groupname]) - set(group.getHosts())
        CURDB.groups.move_hosts(to_add, group)

    if options.apply:
        CURDB.update(smart=True)
    else:
        print red_text("Not updated!!! Add option -y to update.")


if __name__ == '__main__':
    options = get_parser().parse_cmd()
    main(options)
