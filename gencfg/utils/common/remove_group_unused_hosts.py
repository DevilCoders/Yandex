#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import gencfg
from core.argparse.parser import ArgumentParserExt
import core.argparse.types as argparse_types
from core.exceptions import UtilNormalizeException
from core.db import CURDB


def get_parser():
    parser = ArgumentParserExt(description="Remove all unused machines from specified group")
    parser.add_argument("-g", "--group", type=argparse_types.group, required=True,
                        help="Obligatory. Group to process")
    parser.add_argument("-y", "--apply", action="store_true", default=False,
                        help="Optional. Apply changes")

    return parser


def normalize(options):
    if options.group.card.host_donor is not None:
        raise UtilNormalizeException(correct_pfname(__file__), ["group"], "Group <%s> has non-empty host donor <%s>" % (
        options.group.card.name, options.group.card.host_donor))


def main(options):
    all_hosts = set(options.group.getHosts())

    used_hosts = set()
    for intlookup in options.group.card.intlookups:
        intlookup = CURDB.intlookups.get_intlookup(intlookup)
        for instance in intlookup.get_used_instances():
            used_hosts.add(instance.host)

    to_remove = all_hosts - used_hosts

    print "Group %s: total %d hosts, unused %d hosts" % (options.group.card.name, len(all_hosts), len(to_remove))

    for host in to_remove:
        options.group.removeHost(host)

    if options.apply:
        CURDB.groups.update(smart=True)

    return None


def jsmain(d):
    options = get_parser().parse_json(d)
    normalize(options)
    return main(options)


if __name__ == '__main__':
    options = get_parser().parse_cmd()
    result = main(options)
