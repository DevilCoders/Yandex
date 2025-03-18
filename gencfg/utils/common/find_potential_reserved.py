#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))
import hashlib

from argparse import ArgumentParser

import gencfg
from core.db import CURDB
import core.argparse.types as argparse_types


def parse_cmd():
    parser = ArgumentParser(description="Find machines, that can be safely moved from group to reserved")
    parser.add_argument("-g", "--group", dest="group", type=argparse_types.group, required=True,
                        help="Obligatory. Group with potential reserved")
    parser.add_argument("-o", "--hosts", dest="hosts", type=int, required=False,
                        help="Obligatory. Number of potentially reserved hosts")
    parser.add_argument("-r", "--reserved-group", dest="reserved_group", type=argparse_types.group, required=False,
                        help="Optional. Reserved group (with old reserved hosts added to main group)")
    parser.add_argument("-p", "--percents", dest="percents", type=float, required=False,
                        help="Optional. Number of potentially reserved hosts (in percents of main group)")
    parser.add_argument("-f", "--filter", dest="flt", type=argparse_types.pythonlambda, default=lambda x: True,
                        help="Optional. Filter on potentially reserved machines")
    parser.add_argument("-e", "--ignore-slave-groups", type=argparse_types.groups, default=[],
                        help="Optional. List of slave group, which are ignored in filter phase")
    parser.add_argument("-y", "--apply", action="store_true", default=False,
                        help="Optional. Apply changes")

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    options = parser.parse_args()

    if options.hosts is None and options.percents is None:
        raise Exception("You should use one of --hosts or --percents option")
    if options.hosts is not None and options.percents is not None:
        raise Exception("You should use exactly one of --hosts or --percents option")
    if options.reserved_group is None and options.apply is True: \
            raise Exception("You should use --reserved-group with --apply option")

    return options


def main(options):
    # add old reserved to main group
    if options.reserved_group is not None:
        CURDB.groups.move_hosts(options.reserved_group.getHosts(), options.group)
    # calculate number of hosts from percents
    if options.percents is not None:
        options.hosts = int(options.percents / 100. * len(options.group.getHosts()))

    allhosts = options.group.getHosts()
    filteredhosts = filter(lambda x: options.flt(x), allhosts)
    pothosts = list(filteredhosts)

    keephosts = []
    for slave in options.group.slaves:
        if slave.card.host_donor is None and slave not in options.ignore_slave_groups:
            non_suitable, suitable = [], []
            for host in slave.getHosts():
                (non_suitable, suitable)[options.flt(host)].append(host)
            print "Non-donor group %s (%s/%s filtered/suitable)" % (slave.card.name, len(non_suitable), len(suitable))
            keephosts.extend(suitable)
    pothosts = list(set(pothosts) - set(keephosts))

    if len(pothosts) < options.hosts:
        raise Exception("Needed %d hosts, while have only %d" % (options.hosts, len(pothosts)))
    pothosts.sort(cmp=lambda x, y: cmp(hashlib.md5(x.name.partition('.')[0]).hexdigest(),
                                       hashlib.md5(y.name.partition('.')[0]).hexdigest()))

    print "Total %s hosts, %s filtered hosts, %s potential hosts, %s needed hosts" % (
    len(allhosts), len(filteredhosts), len(pothosts), options.hosts)
    print ','.join(map(lambda x: x.name, pothosts[:options.hosts]))

    if options.apply:
        print "Applying changes"
        CURDB.groups.move_hosts(pothosts[:options.hosts], options.reserved_group)
        CURDB.groups.update(smart=True)
    else:
        print "No changes were applied"


if __name__ == '__main__':
    options = parse_cmd()
    main(options)
