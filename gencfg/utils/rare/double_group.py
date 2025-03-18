#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))
import math

from argparse import ArgumentParser

import gencfg
from core.db import CURDB
import core.argparse.types as argparse_types


def parse_cmd():
    parser = ArgumentParser(description="Double group hosts (modify intlookups, modify slaves, ...)")
    parser.add_argument("-g", "--group", dest="group", type=argparse_types.group, required=True,
                        help="Obligatory. Group to proccess")
    parser.add_argument("-s", "--src-group", dest="src_group", type=argparse_types.group, required=True,
                        help="Obligatory. Group with free hosts")

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    options = parser.parse_args()

    if options.group.card.master is not None:
        raise Exception("Group %s is slave group" % options.group)

    return options


def double_list(lst, extrahosts):
    newlst = []
    for instance in lst:
        newhost = extrahosts[instance.host]
        newinstance = CURDB.groups.get_instance_by_N(newhost.name, instance.type, instance.N)
        newlst.append(instance)
        newlst.append(newinstance)
    print lst, newlst
    return newlst


def double_intlookup(intlookup, extrahosts):
    for brigade_group in intlookup.brigade_groups:
        for brigade in brigade_group.brigades:
            for i in range(len(brigade.basesearchers)):
                brigade.basesearchers[i] = double_list(brigade.basesearchers[i], extrahosts)


def double_group(group, extrahosts):
    if group.card.master is None:
        newhosts = map(lambda x: extrahosts[x], group.getHosts())
        CURDB.groups.move_hosts(newhosts, group)
    elif group.card.host_donor is None:
        newhosts = map(lambda x: extrahosts[x], group.getHosts())
        CURDB.groups.add_slave_hosts(newhosts, group)
    for intlookup in group.card.intlookups:
        double_intlookup(CURDB.intlookups.get_intlookup(intlookup), extrahosts)


def main(options):
    # find same hosts
    grouphosts = options.group.getHosts()
    freehosts = set(options.src_group.getHosts())
    newhosts = {}
    for host in grouphosts:
        available = filter(
            lambda x: x.memory == host.memory and math.fabs(x.ssd - host.ssd) < 20 and x.model == host.model, freehosts)
        if len(available) == 0:
            raise Exception("Can not find same host for %s" % host.name)
        print "Found same host: %s -> %s" % (host.name, available[0].name)
        newhosts[host] = available[0]
        freehosts.remove(available[0])

    # add hosts to main group, intlookups
    double_group(options.group, newhosts)
    for slave in filter(lambda x: x.host_donor is None, options.group.slaves):
        double_group(slave, newhosts)
    for slave in filter(lambda x: x.host_donor is not None, options.group.slaves):
        double_group(slave, newhosts)

    CURDB.update()


if __name__ == '__main__':
    options = parse_cmd()
    main(options)
