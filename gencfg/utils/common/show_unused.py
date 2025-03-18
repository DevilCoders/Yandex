#!/skynet/python/bin/python
import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

from argparse import ArgumentParser
from collections import defaultdict

import gencfg
from core.db import CURDB
import core.argparse.types as argparse_types


def parse_cmd():
    parser = ArgumentParser(description="Show unused or partially used hosts in groups")

    parser.add_argument("-g", "--groups", dest="groups", type=argparse_types.xgroups, required=True,
                        help="Obligatory. Comma-separated list of groups (ALLM for all master groups)")
    parser.add_argument("-p", "--partially-used", action="store_true", dest="partially_used", default=False,
                        help="Optional, bool. Show partially used instances")
    parser.add_argument("-s", "--skip-no-intlookup-groups", action="store_true", dest="skip_no_intlookup_groups",
                        default=False,
                        help="Optional, bool. Skip groups with no intlookups")
    parser.add_argument("-v", "--verbose", dest="verbose", action="store_true", default=False,
                        help="Optional, bool. Do verbose output")
    parser.add_argument("-f", "--fail", action="store_true", default=False,
                        help="Optional, bool. Fail when found more than 0 unused machines")
    parser.add_argument("--filter", dest="flt", type=argparse_types.pythonlambda, default=lambda x: True,
                        help="Optional. Lambda-function to filter groups to process")
    parser.add_argument("-u", "--move-unused-to-reserved", action="store_true", default=False,
                        help="Optional. Move all unused machines to reserved")

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    options = parser.parse_args()

    return options


def main(options):
    unused_by_group = defaultdict(list)

    used_instances = defaultdict(lambda: defaultdict(list))
    for intlookup in CURDB.intlookups.get_intlookups():
        for instance in intlookup.get_used_instances():
            used_instances[instance.host][instance.type].append(instance)

    groups = filter(options.flt, options.groups)
    for group in groups:
        if options.skip_no_intlookup_groups and len(group.card.intlookups) == 0:
            continue

        for host in group.getHosts():
            n_all = group.funcs.instanceCount(CURDB, host)
            n_used = len(used_instances[host][group.card.name])

            if options.partially_used:
                unused_instances = set(group.get_host_instances(host)) - set(used_instances[host][group.card.name])
                unused_instances = map(lambda x: '%s:%s:%s:%s' % (x.host.name, x.port, x.power, x.type),
                                       unused_instances)
                if len(unused_instances) > 0:
                    unused_by_group[group].append((host, unused_instances))
            else:
                # check slave groups
                if n_used == 0:
                    for slave in group.slaves:
                        if len(used_instances[host][slave.card.name]) > 0:
                            n_used = 1
                            break

                if n_used == 0:
                    unused_by_group[group].append((host, n_all))

    if options.move_unused_to_reserved:
        for group, hosts_data in unused_by_group.iteritems():
            hosts = map(lambda (x, y): x, hosts_data)
            if group.card.master is None:  # hosts from master group move to reserved
                CURDB.groups.move_hosts_to_reserved(hosts)
                print "Moved %d machines from group %s to reserved" % (len(hosts), group.card.name)
            else:  # hosts from slave groups simply remove
                CURDB.groups.remove_slave_hosts(hosts, group)
                print "Removed %d machines from slave group %s" % (len(hosts), group.card.name)

        CURDB.groups.update(smart=True)

    return unused_by_group


if __name__ == '__main__':
    options = parse_cmd()

    result = main(options)

    if options.verbose:
        for group in sorted(result.keys(), cmp=lambda x, y: cmp(x.card.name, y.card.name)):
            if len(result[group]) > 0:
                print "Group %s:" % group.card.name
                for host, extra_data in result[group]:
                    if options.partially_used:
                        if len(extra_data):
                            print "    %s unused instances: %s" % (host.name, ' '.join(extra_data))
                    else:
                        print "    %s unused %d instances" % (host.name, extra_data)
    else:
        print '\n'.join(sorted(list(set(map(lambda (x, y): x.name, sum(result.values(), []))))))

    if options.fail:
        if len(sum(result.values(), [])) > 0:
            sys.exit(1)
