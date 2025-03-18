import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))

from collections import defaultdict
import copy

from core.db import CURDB


def calc_source_groups(group, recluster_web, verbose=False):
    if group.reqs is None:
        raise Exception('Cannot get host source groups for group "%s". Group "reqs" section is missing.' % group.name)
    locations = group.reqs.hosts.location.location

    src_groups = []
    src_groups.extend(['%s_RESERVED' % location.upper() for location in locations])
    if CURDB.groups.has_group(group.name):
        src_groups.append(group.name)
    if recluster_web:
        src_groups.extend(['%s_WEB_BASE' % location.upper() for location in locations])
    for src_group in src_groups:
        if not CURDB.groups.has_group(src_group):
            raise Exception('Internal error: %s group does not exist' % src_group)
    if verbose:
        print 'Source groups: %s' % ','.join(sorted(src_groups))
    return src_groups


def create_group_host_filter(group, extra_filter=None):
    if group.reqs is None:
        raise Exception('Cannot get host source groups for group "%s". Group "reqs" section is missing.' % group.name)
    reqs = group.reqs
    return create_host_filter(reqs, extra_filter=extra_filter)


def create_host_filter(reqs, extra_filter=None):
    host_filters = []
    if extra_filter is not None:
        host_filters.append(extra_filter)

    # in this function we use pattern:
    #   some_saved_function = lambda host, x=a, y=copy.copy(b): ...
    # this is done to make functions with a copy of 'a' and 'b' values
    # and does not change if 'a' or 'b' changes after it's creation
    # Thanks @kulikov for the solution!

    if reqs.instances.memory_guarantee.megabytes():
        host_filters.append(
            lambda host, min_memory=reqs.instances.memory_guarantee.gigabytes(): host.memory >= min_memory)
    if reqs.instances.ssd.gigabytes():
        host_filters.append(lambda host, min_ssd=reqs.instances.ssd.gigabytes(): host.ssd >= min_ssd)
    if reqs.instances.disk.gigabytes():
        host_filters.append(lambda host, min_disk=reqs.instances.disk.gigabytes(): host.disk >= min_disk)

    # host requirements from reqs.hosts
    if reqs.hosts.ndisks:
        host_filters.append(lambda host, min_n_disks=reqs.hosts.ndisks: host.n_disks >= min_n_disks)
    if reqs.hosts.cpu_models:
        host_filters.append(lambda host, cpu_models=reqs.hosts.cpu_models: not cpu_models or host.model in cpu_models)
    if reqs.hosts.except_cpu_models:
        host_filters.append(lambda host, except_cpu_models=reqs.hosts.except_cpu_models: not except_cpu_models or host.model not in except_cpu_models)

    # host requirements from reqs.hosts.location
    # TODO: validate code required
    scales = ['dc', ]
    scale_to_host_field = {'dc': 'dc',
                           'queue': 'queue',
                           'switch': 'switch',
                           'host': 'name'}
    for scale in scales:
        field = scale_to_host_field[scale]
        if reqs.hosts.location[scale]:
            values = set(reqs.hosts.location[scale])
            host_filters.append(lambda host, field=field, values=copy.copy(values): getattr(host, field) in values)

    return lambda host: all(func(host) for func in host_filters)


def calc_source_hosts(src_groups, src_hosts=None, filter_functon=None, remove_slaves=True, remove_preserved=True,
                      verbose=False):
    del remove_preserved
    if src_hosts is None:
        src_hosts = []
    if src_hosts and isinstance(src_hosts[0], str):
        src_hosts = [CURDB.hosts.get_host_by_name(host) for host in src_hosts]

    if src_groups and isinstance(src_groups[0], str):
        src_groups = [CURDB.groups.get_group(src_group) for src_group in src_groups]
    hosts = set(sum([src_group.getHosts() for src_group in src_groups], []))
    if verbose:
        print 'Number of hosts in source groups: %s' % len(hosts)

    if remove_slaves:
        slaves = sum([src_group.getSlaves() for src_group in src_groups], [])
        # remove dependent slaves
        slaves = [slave for slave in slaves if not slave.hasHostDonor()]
        slave_hosts = set(sum([slave.getHosts() for slave in slaves], []))
        hosts -= slave_hosts
        if verbose:
            print 'Removed source groups slave hosts. Hosts left: %s' % len(hosts)

    if src_hosts:
        hosts |= set(src_hosts)
        print 'Added given specific source hosts. Hosts left: %s' % len(hosts)

    if filter_functon is not None:
        hosts = set(host for host in hosts if filter_functon(host))
        if verbose:
            print 'Applied hosts filter. Hosts left: %s' % len(hosts)

    by_power = defaultdict(int)
    for host in hosts:
        by_power[host.power] += 1

    if verbose:
        by_dc = defaultdict(int)
        for host in hosts:
            by_dc[host.dc] += 1
        print 'Source hosts by dc: %s' % \
              ', '.join('%s: %s' % (dc, by_dc[dc]) for dc in sorted(by_dc.keys()))

    return list(hosts)


def calc_total_shards(group):
    if group.reqs is None:
        raise Exception('Cannot get total shards for group "%s". Group "reqs" section is missing.' % group.name)
    total_shards = group.reqs.shards.fake_shards
    if group.reqs.shards.tiers:
        for tier in set(group.reqs.shards.tiers):
            total_shards += CURDB.tiers.get_total_shards(tier)
    return total_shards


def check_single_instance_on_host(action, group, intlookup=None):
    if group.reqs.instances.max_per_host == 0 or group.reqs.instances.max_per_host > 1:
        raise Exception('Cannot %s for group "%s": implemented for group reqs.instances.max_per_host equal to 1 only.' % (action, group.name))
    if intlookup is not None:
        base_instances = intlookup.get_used_base_instances()
        base_hosts = set(instance.host for instance in base_instances)
        if len(base_instances) != len(base_hosts):
            raise Exception('Cannot %s for group "%s": group intlookup %s has multiple instances on a single host.' % (action, group.name, intlookup.file_name))


def check_trivial_brigade(action, group, intlookup):
    if group.reqs.brigades.size != 1:
        raise Exception('Cannot %s for group "%s": implemented for group reqs.brigades.size equal to 1 only.' % group.name)
    # if group.reqs.brigades.ints is not None:
    #    raise Exception('Cannot %s for group "%s": implemented for group reqs.brigades.ints is None only.')
    if intlookup is not None:
        brigades = [brigade for brigade_group in intlookup.brigade_groups for brigade in brigade_group.brigades]
        has_non_trivial_brigades = not all(
            [len(brigade.basesearchers) == 1 and len(brigade.basesearchers[0]) == 1 for brigade in brigades])
        if has_non_trivial_brigades:
            raise Exception('Cannot %s for group "%s": group intlookup contains non trivial brigades.' % (action, group.name))
        has_ints = not all([len(brigade.intsearchers) == 0 for brigade in brigades])
        if has_ints:
            raise Exception('Cannot %s for group "%s": group intlookup %s contains intsearchers.' % (action, group.name, intlookup.file_name))


def check_single_shard(action, group, intlookup):
    total_shards = calc_total_shards(group)
    if total_shards != 1:
        raise Exception('Cannot %s for group "%s": implemented for group total number of shards equal to 1 only.' % (action, group.name))
    if intlookup is not None:
        if intlookup.get_shards_count() != 1:
            raise Exception('Cannot %s for group "%s": group intlookup %s has number of shards different from 1.' % (action, group.name, intlookup.file_name))
