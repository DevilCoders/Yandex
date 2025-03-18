import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import gencfg  # noqa
from core.db import CURDB as db
from core.instances import TMultishardGroup, TIntGroup, TIntl2Group, Instance
from core.igroups import CIPEntry

from utils.pregen.find_most_memory_free_machines import EUsedInstances, HostInfo, Resources
from utils.pregen.find_most_memory_free_machines import get_busy_instances


def get_host_resources(master):
    busy_instances = get_busy_instances(master.getHosts(), EUsedInstances.ASSIGNED)
    host_infos = {host: HostInfo(host) for host in master.getHosts()}
    for instance in busy_instances:
        if instance.host in host_infos:
            host_infos[instance.host].append_instance(instance)
    return host_infos


def get_group_resources(group):
    try:
        instance_power = float(group.card.legacy.funcs.instancePower.replace('exactly', ''))
    except ValueError:
        instance_power = 0.0

    return Resources(power=instance_power,
                     hdd=group.card.reqs.instances.disk.gigabytes(),
                     ssd=group.card.reqs.instances.ssd.gigabytes(),
                     mem=group.card.reqs.instances.memory_guarantee.gigabytes())


def build_intlookup(group, instances):
    if db.intlookups.has_intlookup(group.card.name):
        intl = db.intlookups.get_intlookup(group.card.name)
    else:
        intl = db.intlookups.create_empty_intlookup(group.card.name)

    intl.base_type = group.card.name
    intl.file_name = group.card.name
    intl.hosts_per_group = len(instances)
    intl.brigade_groups_count = 1
    intl.tiers = None

    intl2_group = TIntl2Group()
    basesearchers = []
    for instance in instances:
        basesearchers.append([instance])
    int_group = TIntGroup(basesearchers, [])

    multishard_group = TMultishardGroup()
    multishard_group.brigades.append(int_group)
    intl2_group.multishards.append(multishard_group)
    intl.intl2_groups = [intl2_group]

    # db.intlookups.add_build_intlookup(intl)
    group.card.intlookups = [group.card.name]
    group.mark_as_modified()
    return intl


def empty_group(group):
    for intlookup_name in group.card.intlookups:
        db.intlookups.remove_intlookup(intlookup_name)
    group.custom_instance_power.clear()
    group.clearHosts()
    #db.groups.remove_slave_hosts(group.getHosts(), group)
    group.mark_as_modified()


def main(master, cohabitants):
    master = db.groups.get_group(master)
    hosts = get_host_resources(master)

    groups = [db.groups.get_group(group) for group in cohabitants]
    for group in groups:
        empty_group(group)
    db.update(smart=True)

    for group in groups:
        instances = []
        hostnames = set()
        request = get_group_resources(group)
        port = int(group.card.legacy.funcs.instancePort.split('new')[1])
        print group.card.name, 'requires', request

        for hostname, host in hosts.iteritems():
            for port_offset in xrange(20):
                if host.fits(request):
                    instance = Instance(host.host, request.power, port + port_offset * 8, group.card.name, 0)
                    instances.append(instance)
                    host.append_instance(instance)
                    group.custom_instance_power[CIPEntry(instance)] = request.power
                    hostnames.add(host.host.name)
                else:
                    print hostname, 'accepts', port_offset, group.card.name
                    break

        intlookup = build_intlookup(group, instances)
        intlookup.write_intlookup_to_file_json()
        with open(group.card.name + '.hosts', 'w') as f:
            for host in sorted(hostnames):
                print >> f, host
        with open(group.card.name + '.instances', 'w') as f:
            for instance in sorted(instances):
                print >> f, '{}:{} {}'.format(instance.host.name, instance.port, instance.power)


if __name__ == '__main__':
    main('SAS_MARKET_TEST_GENERAL', ['SAS_MARKET_TEST_GENERAL_YP_CPU',
                                     'SAS_MARKET_TEST_GENERAL_YP_DISK',
                                     'SAS_MARKET_TEST_GENERAL_YP_SSD',
                                     'SAS_MARKET_TEST_GENERAL_YP_MEM'
                                    ])
