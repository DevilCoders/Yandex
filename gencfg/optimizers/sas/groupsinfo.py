import collections
import copy
import functools
import logging
import random

from core.db import CURDB
from core.instances import Instance

class HostsGroup(object):
    def __init__(self, hosts, group, power, custom_icount=None):
        self.hosts = hosts
        self.group = group
        self.icount = custom_icount if custom_icount else len(self.group.get_host_instances(self.hosts[0]))
        self.power = power
        self.ssd = False

        self.igroups = []

    def __str__(self):
        return 'Hostgroup on host {}: {} instances, {:.2f} power'.format(self.hosts[0].name, self.icount, self.power)


class InstancesGroup(object):
    def __init__(self, hgroup, power, slotsize):
        self.hgroup = hgroup
        self.power = power
        self.slotsize = slotsize

    def swap_data(self, other):
        for i in range(len(self.hgroup.igroups)):
            if self.hgroup.igroups[i] == self:
                self.hgroup.igroups[i] = other
        for i in range(len(other.hgroup.igroups)):
            if other.hgroup.igroups[i] == other:
                other.hgroup.igroups[i] = self

        self.hgroup, other.hgroup = other.hgroup, self.hgroup
        self.power, other.power = other.power, self.power


def load_hgroups(group_name, hosts_per_group, separate_ssd=True, flt=None, verbose=False, exclude_cpu_from=(), exclude_hosts_from=()):
    exclude_hosts = set()
    for group in exclude_hosts_from:
        for host in group.getHosts():
            exclude_hosts.add(host.name)

    group = CURDB.groups.get_group(group_name)
    hosts = group.getHosts()

    if flt is not None:
        hosts = filter(lambda x: flt(x), hosts)

    hosts = filter(lambda x: x.name not in exclude_hosts, hosts)

    # ========================= RX-400 (cpu from given groups) START =====================================
    reduce_cpu_by = collections.defaultdict(float)
    for exclude_group in exclude_cpu_from:
        for instance in exclude_group.get_kinda_busy_instances():
            reduce_cpu_by[instance.host] += instance.power
    # ========================= RX-400 STOP ======================================

    # some nonsense, never used
    if separate_ssd:
        hgroups, free_hosts = load_hgroups_separate_ssd(hosts, group, hosts_per_group, reduce_cpu_by)
    # common case
    else:
        hgroups, free_hosts = load_hgroups_internal(hosts, group, hosts_per_group, reduce_cpu_by)

    print 'Sample hgroups:'
    print hgroups[42]
    print hgroups[-42]

    free_hosts.sort(cmp=functools.partial(sort_hosts, group, reduce_cpu_by))
    wasted_hosts = filter(lambda x: len(group.get_host_instances(x)) < 1, free_hosts)
    free_hosts = filter(lambda x: len(group.get_host_instances(x)) >= 1, free_hosts)

    wasted_instances = []
    for i in range(len(free_hosts) / hosts_per_group):
        count = min(map(lambda x: len(group.get_host_instances(x)), free_hosts[:hosts_per_group]))
        power = min(
            map(lambda host: sum(map(lambda x: x.power, group.get_host_instances(host))) - reduce_cpu_by[host], free_hosts[:hosts_per_group]))

        for free_host in free_hosts[:hosts_per_group]:
            free_instances_N = len(group.get_host_instances(free_host)) - count
            # host_wasted_instances = map(lambda x: copy.copy(x), group.get_host_instances(free_host)[:free_instances_N])
            host_wasted_instances = map(lambda x: Instance(x.host, x.power, x.port, x.type, x.N), group.get_host_instances(free_host)[:free_instances_N])
            for instance in host_wasted_instances:
                instance.power = (free_host.power - power) / free_instances_N
            wasted_instances.extend(host_wasted_instances)

        # print (power, count), [x.name for x in free_hosts[:hosts_per_group]]
        hgroups.append(HostsGroup(free_hosts[:hosts_per_group], group, power, custom_icount=count))
        free_hosts = free_hosts[hosts_per_group:]

    wasted_instances.extend(sum(map(lambda x: group.get_host_instances(x), free_hosts + wasted_hosts), []))
    wasted_power = sum(
        map(lambda x: sum(map(lambda x: x.power, group.get_host_instances(x))), free_hosts + wasted_hosts))

    print ('Created host groups:\n'
           '    Host groups {hgroups_count}, instances groups {igroups_count}\n'
           '    Host groups with ssd {hgroups_ssd_count}, instances groups with ssd {igroups_ssd_count}\n'
           '    Total power {hgroups_power:.3f}\n'
           '    Wasted power {wasted_power:.3f}\n'
           '    Wasted instances {wasted_instances}').format(
                   hgroups_count=len(hgroups), igroups_count=sum(map(lambda x: x.icount, hgroups)), hgroups_ssd_count=len(filter(lambda x: x.ssd == True, hgroups)),
                   igroups_ssd_count=sum(map(lambda x: x.icount, filter(lambda x: x.ssd == True, hgroups))), hgroups_power=sum(x.power for x in hgroups),
                   wasted_power=wasted_power, wasted_instances=len(wasted_instances))
    if verbose:
        print 'Total {} hgroups:'.format(len(hgroups))
        for hgroup in hgroups:
            print '   {}'.format(hgroup)


    wasted_instances_by_host = collections.defaultdict(int)
    wasted_power_by_host = collections.defaultdict(float)
    for instance in wasted_instances:
        wasted_instances_by_host[instance.host] += 1
        wasted_power_by_host[instance.host] += instance.power

    print "Wasted resources by host:"
    for host in wasted_instances_by_host.iterkeys():
        print "    Host %s: %d instances, %f power" % (
        host.name, wasted_instances_by_host[host], wasted_power_by_host[host])

#    hgroups.sort(cmp = lambda x, y: cmp((y.ssd, x.icount), (x.ssd, y.icount)))
#    hgroups.sort(cmp = lambda x, y: cmp(x.icount, y.icount))
#    hgroups.sort(cmp = lambda x, y: cmp(y.icount, x.icount))
#    hgroups.sort(cmp=lambda x, y: cmp((y.ssd, y.icount), (x.ssd, x.icount)))
    hgroups.sort(cmp=lambda x, y: cmp((x.icount > 2) * 1000 - x.icount, (y.icount > 2) * 1000 - y.icount))
#    random.shuffle(hgroups)

    hgroups = filter(lambda x: x.icount > 0, hgroups)

    if separate_ssd and len(filter(lambda x: x.ssd > 0, hgroups)) == 0:
        raise Exception("Trying to optimize ssd without ssd replicas")

    return hgroups


def load_hgroups_internal(hosts, group, hosts_per_group, reduce_cpu_by):
    filtered_hosts = collections.defaultdict(list)

    # why sort by switch here?
    for host in sorted(hosts, cmp=lambda x, y: cmp(x.switch, y.switch)):
        host_power = sum(instance.power for instance in group.get_host_instances(host)) - reduce_cpu_by[host]
        if host_power > 0:
            filtered_hosts[(host_power, len(group.get_host_instances(host)))].append(host)

# ==== logging ====
    print 'Sample filtered hosts'
    s = 0
    for k, v in sorted(filtered_hosts.iteritems()):
        print '{:.2f}, {}: {}'.format(k[0], k[1], len(v))
        s += len(v) * k[1]
    print 'Total slots:', s
# =================

    host_groups = []
    leftovers = []
    for power_count, hosts_ in filtered_hosts.iteritems():
        # cut into groups of {hosts_per_group} identical hosts
        while len(hosts_) >= hosts_per_group:
            logging.debug('%s %s', power_count, [host.name for host in hosts_[:hosts_per_group]])

            host_groups.append(HostsGroup(hosts_[:hosts_per_group], group, power_count[0]))
            hosts_ = hosts_[hosts_per_group:]

        if len(hosts_) > 0:
            logging.debug('    Adding {} hosts with power {} and instances {} to leftovers ({})', len(hosts_), power_count[0], power_count[1], ','.join([x.name for x in hosts_]))
        leftovers.extend(hosts_)

    return host_groups, leftovers


def load_hgroups_separate_ssd(hosts, group, hosts_per_group, reduce_cpu_by):
    print "Distributing ssd hosts:"
    hgroups, free_hosts = load_hgroups_internal(filter(lambda x: x.ssd != 0, hosts), group, hosts_per_group, reduce_cpu_by)

    print "Free ssd hosts:"
    print "\n".join(
        map(lambda x: "    Host %s: %d instances, %s power" % (x.name, len(group.get_host_instances(x)), x.power),
            free_hosts))

    for hgroup in hgroups:
        hgroup.ssd = True

    print "Distributing nossd hosts:"
    nossd_hgroups, nossd_free_hosts = load_hgroups_internal(filter(lambda x: x.ssd == 0, hosts) + free_hosts, group,
                                                            hosts_per_group, reduce_cpu_by)
    hgroups += nossd_hgroups
    free_hosts = nossd_free_hosts

    return hgroups, free_hosts


def sort_hosts(group, reduce_cpu_by, host1, host2):
    p1, c1 = sum(map(lambda x: x.power, group.get_host_instances(host1))), len(group.get_host_instances(host1))
    p2, c2 = sum(map(lambda x: x.power, group.get_host_instances(host2))), len(group.get_host_instances(host2))

    p1 -= reduce_cpu_by[host1]
    p2 -= reduce_cpu_by[host2]


    if c1 < c2: return 1
    if c1 > c2: return -1

    if p1 - p2 < -0.1: return 1  # FIXME: float !!
    if p1 - p2 > 0.1: return -1  # FIXME: float !!

    return 0
