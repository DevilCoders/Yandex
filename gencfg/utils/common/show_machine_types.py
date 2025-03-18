#!/skynet/python/bin/python
import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

from collections import defaultdict
from argparse import ArgumentParser

import gencfg
from core.db import CURDB
import core.argparse.types as argparse_types


def parse_cmd():
    parser = ArgumentParser(description="Show machines types for list of machines")
    parser.add_argument("-s", "--hosts", dest="hosts", type=argparse_types.hostnames, default=None,
                        help="Optional. Comma-separated list of hosts or groups")
    parser.add_argument("-n", "--no-slaves", action="store_true", dest="no_slaves", default=False,
                        help="Optional, bool. Do not show slave groups, fake groups or background groups")
    parser.add_argument("--no-background", action="store_true", default=False,
                        help="Optional. bool. Do not show background groups")
    parser.add_argument("-u", "--show-unassigned", action="store_true", default=False,
                        help="Optional, bool. Show only hosts unassigned to any group")
    parser.add_argument("-f", "--filter", dest="host_filter", type=argparse_types.pythonlambda, default=None,
                        help="Optional. Host by filter")
    parser.add_argument("--fail-on-error", action="store_true", default=False,
                        help="Optional. Exit with non-zero status with there are unassigned/unknown hosts")

    if len(sys.argv) == 1:
        sys.argv.append('-h')

    options = parser.parse_args()

    if options.hosts is None:
        if options.host_filter is None:
            options.hosts = map(lambda x: x.name, CURDB.hosts.get_all_hosts())
        else:
            options.hosts = map(lambda x: x.name, filter(lambda x: options.host_filter(x), CURDB.hosts.get_all_hosts()))

    return options


if __name__ == '__main__':
    retcode = 0

    options = parse_cmd()

    unknown_hosts = set()
    result = defaultdict(list)
    for host in options.hosts:
        if CURDB.hosts.has_host(host):
            for group in CURDB.groups.get_host_groups(host):
                result[group].append(host)
        else:
            unknown_hosts.add(host)

    orphan_hosts = set(options.hosts)
    for group in sorted(result.keys(), cmp=lambda x, y: cmp(x.card.name, y.card.name)):
        if options.no_slaves:
            if group.card.master is not None:
                continue
            if group.card.on_update_trigger is not None:
                continue
            if group.card.properties.background_group:
                continue
        if options.no_background and group.card.properties.background_group:
            continue


        group_size = len(group.getHostNames())
        group_hosts = sorted(list(set(result[group])))
        orphan_hosts -= set(result[group])
        if not options.show_unassigned:
            print "%s ( %d total of %d ): %s" % (group.card.name, len(group_hosts), group_size, ','.join(group_hosts))

    orphan_hosts -= unknown_hosts
    if orphan_hosts:
        print "<no group> ( %d total ): %s" % (len(orphan_hosts), ','.join(sorted(list(orphan_hosts))))
        if options.fail_on_error:
            retcode = 1

    if unknown_hosts:
        print "<unknown hosts> ( %d total ): %s" % (len(unknown_hosts), ','.join(sorted(list(unknown_hosts))))
        if options.fail_on_error:
            retcode = 1

    sys.exit(retcode)
