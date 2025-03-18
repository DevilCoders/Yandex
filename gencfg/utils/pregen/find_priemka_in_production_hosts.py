#!/skynet/python/bin/python

import os
import sys
import string
import math

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))
from optparse import OptionParser

import gencfg
from core.db import CURDB


def parse_cmd():
    usage = "Usage %prog [options]"
    parser = OptionParser(usage=usage)
    parser.add_option("-r", "--replicas", type="float", dest="replicas",
                      help="obligatory option. Total number of replicas")
    parser.add_option("-i", "--instances", type="str", dest="instances",
                      help="obligatory option. Number of shards (instances)")
    parser.add_option("-g", "--group", type="str", dest="group",
                      help="obligatory option. Group to get hosts from")
    parser.add_option("-s", "--slave-group", type="str", dest="slave_group",
                      help="obligatory option. Slave group to add hosts to")
    parser.add_option("-t", "--split-by-switch-type", action="store_true", dest="split_by_switch_type", default=False,
                      help="Optional. Make half of instances in one switch type, other half - in other")
    parser.add_option("-x", "--excluded-groups", type="str", dest="excluded_groups",
                      help="comma separated groups which hosts are excluded from set of source hosts")
    parser.add_option("-f", "--filter", type="str", dest="host_filter", default=None,
                      help="Optional. Host filter")
    parser.add_option("-R", "--reliable-turkey", action="store_true", dest="reliable_turkey", default=None,
                      help="Optional. Special reliable Turkey option")

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    (options, args) = parser.parse_args()

    if options.replicas is None:
        raise Exception("Option --relicas is obligatory")
    if options.instances is None:
        raise Exception("Option --instances is obligatory")
    if options.group is None:
        raise Exception("Option --group is obligatory")
    if options.slave_group is None:
        raise Exception("Option --slave-group is obligatory")
    if options.replicas == 1 and options.split_by_switch_type == True:
        raise Exception("Can not switch by switch type with one replica")

    options.group = CURDB.groups.get_group(options.group)
    options.slave_group = CURDB.groups.get_group(options.slave_group)
    if CURDB.groups.has_group(string.replace(options.slave_group.name, '_BASE', '_INT')):
        options.slave_group_int = CURDB.groups.get_group(string.replace(options.slave_group.name, '_BASE', '_INT'))
    else:
        options.slave_group_int = None
    options.instances = CURDB.tiers.primus_int_count(options.instances)[0]

    if options.slave_group.master != options.group:
        raise Exception("Group %s is not master for group %s" % (options.group, options.slave_group))

    if options.excluded_groups is None:
        options.excluded_groups = []
    else:
        options.excluded_groups = options.excluded_groups.split(',')
    options.host_filter = eval(options.host_filter) if options.host_filter is not None else None

    return options


def add_slave_hosts(hosts, total_instances, group):
    for host in hosts[:total_instances]:
        CURDB.groups.add_slave_host(host, group)


if __name__ == '__main__':
    options = parse_cmd()

    # FIXME: hack
    options.slave_group.clearHosts()

    preserved_hosts = set()
    for excluded_group in options.excluded_groups:
        excluded_group_hosts = CURDB.groups.get_group(excluded_group).getHosts()
        excluded_group_hosts = set(excluded_group_hosts)
        preserved_hosts |= excluded_group_hosts

    hosts = list(set(options.group.getHosts()) - preserved_hosts)
    if options.host_filter:
        hosts = filter(lambda x: options.host_filter(x), hosts)


    def custom_sort(x, y):
        vx = options.group.funcs.instanceCount(CURDB, x)
        vy = options.group.funcs.instanceCount(CURDB, y)
        if vx != vy:
            return cmp(vy, vx)
        #            if vx > 1 and vy > 1:
        #                return cmp(vx, vy)
        #            return cmp(vy, vx)
        if x.switch != y.switch:
            return cmp(x.switch, y.switch)
        return cmp(x.name, y.name)


    hosts.sort(cmp=custom_sort)

    # TODO: test that there are at least 1 instance of WEB on machine

    total_instances = int(math.ceil(options.replicas * options.instances))
    print "Have %d hosts, needed %d hosts" % (len(hosts), total_instances)
    if len(hosts) < total_instances:
        raise Exception("Not enough hosts for creating priemka in group %s: have %s, needed %s" % (options.group.card.name, len(hosts), total_instances))
    if options.group.funcs.instanceCount(CURDB, hosts[total_instances - 1]) == 1:
        raise Exception("Not enough hosts with more than 1 replica in group %s" % options.group.card.name)

    if options.reliable_turkey:
        add_slave_hosts(filter(lambda x: x.switch_type == 0 and x.dc in ['ugrb'], hosts), total_instances / 4,
                        options.slave_group)
        add_slave_hosts(filter(lambda x: x.switch_type == 1 and x.dc in ['ugrb'], hosts), total_instances / 4,
                        options.slave_group)

        fol_myt_size = total_instances / 4
        fol_size = fol_myt_size / 4 * 3
        myt_size = fol_myt_size / 4 * 1

        add_slave_hosts(filter(lambda x: x.switch_type == 0 and x.dc in ['fol'], hosts), fol_size, options.slave_group)
        add_slave_hosts(filter(lambda x: x.switch_type == 1 and x.dc in ['fol'], hosts), fol_size, options.slave_group)

        add_slave_hosts(filter(lambda x: x.switch_type == 0 and x.dc in ['myt'], hosts), myt_size, options.slave_group)
        add_slave_hosts(filter(lambda x: x.switch_type == 1 and x.dc in ['myt'], hosts), myt_size, options.slave_group)

    elif options.split_by_switch_type:
        add_slave_hosts(filter(lambda x: x.switch_type == 0, hosts), total_instances / 2, options.slave_group)
        add_slave_hosts(filter(lambda x: x.switch_type == 1, hosts), total_instances / 2, options.slave_group)
    else:
        add_slave_hosts(hosts, total_instances, options.slave_group)

    found_hosts = len(options.slave_group.getHosts())
    required_hosts = total_instances
    if found_hosts != required_hosts:
        assert (found_hosts < required_hosts)
        raise Exception('Not enough hosts for group. Need %s has %s' % (required_hosts, found_hosts))

    CURDB.groups.update()
