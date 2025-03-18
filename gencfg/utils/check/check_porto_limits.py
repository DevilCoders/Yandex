#!/skynet/python/bin/python

import os
import sys
# import multiprocessing

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import time
from argparse import ArgumentParser
from collections import defaultdict
import cProfile
import StringIO
import pstats

import gencfg
from core.db import CURDB
import core.argparse.types as argparse_types
from core.instances import FakeInstance


GB = 1024 * 1024 * 1024
MB = 1024 * 1024


def hr_memory(v):
    if v > 100 * MB:
        return "%.2fGb" % (float(v) / GB)
    else:
        return "%.2fMb" % (float(v) / MB)


class InstanceWithLimit(object):
    __slots__ = ['instance', 'memory', 'ssd', 'hdd', 'net']

    def __init__(self, instance, memory, ssd, hdd, net):
        self.instance = instance
        self.memory = memory
        self.ssd = ssd
        self.hdd = hdd
        self.net = net

        if (instance.host.ssd > 0) and (instance.host.disk == 0):
            # there are a lot of hosts with ssd only. For such hosts we treat hdd requirements as ssd requirements
            self.ssd = self.ssd + self.hdd
            self.hdd = 0

    def as_str(self):
        if isinstance(self.instance, FakeInstance):
            tp = 'NONE'
        else:
            tp = self.instance.type
        return "{}:{}:{} cpu={:.2f} memory={} ssd={} hdd={} net={}".format(self.instance.host.name, self.instance.port, tp, self.instance.power,
                                                                           hr_memory(self.memory), hr_memory(self.ssd), hr_memory(self.hdd), hr_memory(self.net))


def parse_cmd():
    parser = ArgumentParser(description="Check consistency of custom instances power")

    parser.add_argument("-g", "--groups", type=argparse_types.groups, default=None,
                        help="Optional. List of master-groups to check")
    parser.add_argument("-f", "--filter", type=argparse_types.pythonlambda, default=lambda x: True,
                        help="Optional. Filter on groups")
    parser.add_argument('--check-ssd', action='store_true', default=False,
                        help='Optional. Check ssd overcommit as well')
    parser.add_argument('--check-hdd', action='store_true', default=False,
                        help='Optional. Check hdd overcommit as well')
    parser.add_argument('--check-net', action='store_true', default=False,
                        help='Optional. Check net overcommit as well (RX-430)')

    if len(sys.argv) == 1:
        sys.argv.append('-h')

    return parser.parse_args()


def load_by_host_data(options):
    hosts_data = defaultdict(list)

    master_groups = filter(options.filter, options.groups)
    master_groups = filter(lambda x: x.card.master is None, master_groups)
    master_groups_hosts = set()
    for group in master_groups:
        master_groups_hosts |= set(group.getHosts())

    background_groups = filter(lambda x: ((x.card.on_update_trigger is not None) or (x.card.properties.background_group)) and (x.card.master is None), CURDB.groups.get_groups())
    background_groups = filter(lambda x: x not in master_groups, background_groups)
    master_groups.extend(background_groups)

    groups_with_slaves = sum(map(lambda x: x.slaves, master_groups), []) + master_groups
    for group in groups_with_slaves:
        memory_value = group.card.reqs.instances.memory_guarantee.value
        ssd_value = group.card.reqs.instances.ssd.value
        disk_value = group.card.reqs.instances.disk.value
        net_value = group.card.reqs.instances.net_guarantee.value

        for instance in group.get_kinda_busy_instances():
            hosts_data[instance.host].append(InstanceWithLimit(instance, memory_value, ssd_value, disk_value, net_value))

    # filter out non-master group instances
    hosts_data = {x: y for x, y in hosts_data.iteritems() if x in master_groups_hosts}

    return hosts_data


def check_cpu(options):
    """Check for cpu limit constraints"""

    exclude_groups = [
        'SAS_MARKET_TEST_GENERAL', 'SAS_MARKET_PROD_GENERAL',  # temporary add market groups with cpu overcommit
        'MAN_SAAS_CLOUD', 'SAS_SAAS_CLOUD', 'VLA_SAAS_CLOUD',  # temporary remove check for overcommit (@i024)
    ]

    exclude_hosts = []
    for group in CURDB.groups.get_groups():
        if group.card.name in exclude_groups:
            exclude_hosts += group.getHosts()
    exclude_hosts = set(exclude_hosts)

    interested_hosts = defaultdict(list)
    for group in options.groups:
        for host in group.getHosts():
            if host not in exclude_hosts:
                interested_hosts[host] = []

    for group in CURDB.groups.get_groups():
        # skip groups with idle cpu
        if group.card.reqs.instances.get('cpu_policy') == 'idle':
            continue

        # skip full host group
        if group.card.properties.full_host_group:
            continue

        # skip portovm guest groups
        if group.card.properties.created_from_portovm_group:
            continue

        if (group.card.legacy.funcs.instancePower in ('default', 'zero', 'exactly0')) and \
           (group.custom_instance_power_used == False):
            continue

        for instance in group.get_kinda_busy_instances():
            # skip not interested hosts
            if instance.host in interested_hosts:
                interested_hosts[instance.host].append(instance)

    is_failed = False
    for host, instances in interested_hosts.iteritems():
        used_power = sum(x.power for x in instances)
        if used_power > host.power:
            print 'Host {} have {:.2f} power, while {:.2f} needed by instances:\n' \
                  '{}'.format(host.name, host.power, used_power, '\n'.join(('    {}'.format(x) for x in instances)))
            is_failed = True

    return is_failed


def check_memory_ssd_hdd_net(options):
    """
        Check porto constraints:
            - check if full_host_group does not have any child groups
            - check if fake_group is master group
    """

    is_failed = False

    # check groups without memory limits and non master fake groups
    no_memory_limit_groups = set()
    non_master_fake_groups = set()
    sdd_io_limit_hosts = defaultdict(set)
    for group in CURDB.groups.get_groups():
        if group.card.properties.fake_group:
            if group.card.master is not None:
                non_master_fake_groups.add(group.card.name)
            continue
        reqs = group.card.reqs.instances
        if reqs.ssd_io_read_limit or reqs.ssd_io_write_limit or reqs.ssd_io_ops_read_limit or reqs.ssd_io_ops_write_limit:
            for host in group.getHosts():
                sdd_io_limit_hosts[host].add(group)
        if group.card.properties.full_host_group or group.card.reqs.instances.memory_guarantee.value > 0:
            continue
        no_memory_limit_groups.add(group.card.name)

    if non_master_fake_groups:
        print "Non-master fake groups: %s" % ",".join(non_master_fake_groups)
        is_failed = True

    if no_memory_limit_groups:
        print "Groups without memory limits: %s" % ",".join(no_memory_limit_groups)
        is_failed = True

    # check porto limits
    hosts_data = load_by_host_data(options)
    for host in hosts_data:
        if host.memory == 0 and len(hosts_data[host]) == 0:  # FIXME: temporary hack until we can detect memory
            continue

        have_memory = host.get_avail_memory()
        used_memory = sum(map(lambda x: x.memory, hosts_data[host]))
        if used_memory > have_memory:
            print "Host %s have %s memory, while need %s by instances:\n%s"\
                  % (host.name, hr_memory(have_memory), hr_memory(used_memory), "\n".join(map(lambda x: "    %s" % x.as_str(), hosts_data[host])))
            is_failed = True

        if options.check_ssd:
            have_ssd = host.ssd * GB
            used_ssd = sum(map(lambda x: x.ssd, hosts_data[host]))
            if used_ssd > have_ssd:
                print "Host %s have %s ssd, while need %s by instances:\n%s"\
                      % (host.name, hr_memory(have_ssd), hr_memory(used_ssd), "\n".join(map(lambda x: "    %s" % x.as_str(), hosts_data[host])))
                is_failed = True
            if not host.ssd and host in sdd_io_limit_hosts:
                print('Host {} have not ssd, but groups {} need sdd io limit'.format(
                    host.name, ','.join([x.card.name for x in sdd_io_limit_hosts[host]])
                ))
                is_failed = True

        if options.check_hdd:
            have_hdd = host.disk * GB
            used_hdd = sum(map(lambda x: x.hdd, hosts_data[host]))
            if used_hdd > have_hdd:
                print "Host %s have %s hdd, while need %s by instances:\n%s"\
                      % (host.name, hr_memory(have_hdd), hr_memory(used_hdd), "\n".join(map(lambda x: "    %s" % x.as_str(), hosts_data[host])))
                is_failed = True

        if options.check_net:
            have_net = host.net * MB
            used_net = sum(map(lambda x: x.net, hosts_data[host]))
            if used_net > have_net:
                print "Host %s have %s net, while need %s by instances:\n%s"\
                      % (host.name, hr_memory(have_net), hr_memory(used_net), "\n".join(map(lambda x: "    %s" % x.as_str(), hosts_data[host])))
                is_failed = True

    return is_failed


if __name__ == '__main__':
    options = parse_cmd()

    cpu_status = check_cpu(options)
    mem_status = check_memory_ssd_hdd_net(options)

    sys.exit(int(cpu_status or mem_status))
