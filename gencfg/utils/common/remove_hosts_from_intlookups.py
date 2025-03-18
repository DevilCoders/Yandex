#!/skynet/python/bin/python
import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

from optparse import OptionParser

import gencfg
from core.db import CURDB


def parse_cmd():
    usage = "Usage: %prog [options]"
    parser = OptionParser(usage=usage)

    parser.add_option("-s", "--hosts", dest="hosts", default=None,
                      help="Obligatory. Hosts to remove from intlookups")
    parser.add_option("-i", "--ignore-empty", action="store_true", dest="ignore_empty", default=False,
                      help="Optional. Do not throw exception on empty groups")

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    (options, args) = parser.parse_args()

    if options.hosts is None:
        raise Exception("Option --hosts is obligatory")

    if len(args):
        raise Exception("Unparsed args: %s" % args)

    options.hosts = map(lambda x: CURDB.hosts.get_host_by_name(x), options.hosts.split(","))

    return options


def remove_hosts_from_intlookups(options):
    intlookups = CURDB.intlookups.get_intlookups()

    search_hosts = set(options.hosts)

    for intlookup in intlookups:
        removed_hosts = set()
        removed_other = set()

        min_before_power = sys.float_info.max
        min_after_power = sys.float_info.max

        for brigade_group in intlookup.brigade_groups:
            min_before_power = min(min_before_power, sum(map(lambda x: x.power, brigade_group.brigades)))

            new_brigades = []
            for brigade in brigade_group.brigades:
                brigade_hosts = set(map(lambda x: x.host, brigade.get_all_instances()))

                if len(search_hosts & brigade_hosts):
                    removed_hosts |= search_hosts & brigade_hosts
                    removed_other |= brigade_hosts
                else:
                    new_brigades.append(brigade)
            brigade_group.brigades = new_brigades
            if not len(new_brigades) and not options.ignore_empty:
                raise Exception("Got empty group while processing %s" % intlookup.file_name)

            min_after_power = min(min_after_power, sum(map(lambda x: x.power, brigade_group.brigades)))

        if len(removed_hosts):
            print "Intlookup %s: removed %d hosts with %d other hosts (before power - %s, after power %s)" % \
                  (intlookup.file_name, len(removed_hosts), len(removed_other), min_before_power, min_after_power)


if __name__ == '__main__':
    options = parse_cmd()
    remove_hosts_from_intlookups(options)
    CURDB.update()
