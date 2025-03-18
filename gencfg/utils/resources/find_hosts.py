#!/skynet/python/bin/python
# coding: utf8
from __future__ import print_function

import os
import sys
import time
import copy
import logging
import functools
import collections

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import gencfg
import core.argparse.types as argparse_types


from argparse import ArgumentParser
from core.db import CURDB
from optimizers.dynamic.aux_dynamic import calc_instance_memory


DEFAULT_SRC_GROUP = 'MAN_RESERVED,SAS_RESERVED,VLA_RESERVED,MSK_RESERVED'
DEFAULT_DC = 'man,sas,vla,myt,iva'
DEFAULT_ORDERING = 'memory,power,ssd,net,hdd'


def work_time(func):
    @functools.wraps(func)
    def wrapper(*args, **kwargs):
        start = time.time()
        r_value = func(*args, **kwargs)
        logging.debug('WORKTIME {} {}s'.format(func.__name__, time.time() - start))
        return r_value
    return wrapper


def get_list_form_comma_list(comma_list):
    result_list = []

    if comma_list.startswith('#'):
        filename = comma_list[1:]
        with open(filename, 'r') as data:
            for line in data:
                result_list.append(line.strip())
    else:
        result_list = [x.strip() for x in comma_list.split(',')]

    return result_list


def groups_hosts(groups):
    return set(sum(map(lambda g: g.getHosts(), groups), []))


def group_slaves(group):
    return group.card.slaves


def group_donor_slaves(group):
    donors = {x for x in group_slaves(group) if x.host_donor}
    return {x for x in group_slaves(group) if x.host_donor in donors} | {group}


def host_free_resources(host):
    free_resources = {
        'host': host,
        'power': int(host.power),
        'memory': host.get_avail_memory(),
        'hdd': host.disk,
        'ssd': host.ssd,
        'net': host.net / 8.,
    }

    for instance in CURDB.groups.get_host_instances(host):
        if instance.type in map(lambda x: '{}_RESERVED'.format(x), ['MAN', 'SAS', 'VLA', 'MSK']):
            continue
        reqs = CURDB.groups.get_group(instance.type).card.reqs

        free_resources['power'] -= int(instance.power)
        free_resources['memory'] -= calc_instance_memory(CURDB, instance)
        free_resources['hdd'] -= reqs.instances.disk.gigabytes()
        free_resources['ssd'] -= reqs.instances.ssd.gigabytes()
        free_resources['net'] -= reqs.instances.net_guarantee.megabytes()
    free_resources['memory'] /= 1024. * 1024 * 1024

    return free_resources


def get_groups_sum_guarantees(groups):
    def group_power(group):
        card_power = 0
        power = group.card.legacy.funcs.instancePower
        if power == 'fullhost' or group.card.properties.full_host_group:
            return 1
        elif power.startswith('exactly'):
            card_power = int(power.replace('exactly', ''))

        inst_power = max(map(lambda x: int(x.power), group.get_instances()) or [0])
        return max(card_power, inst_power)

    def max_switch_distribution(group, distribution):
        current_distribution = collections.defaultdict(int)
        for host in group.getHosts():
            current_distribution[host.switch] += 1

        max_distribution = collections.defaultdict(int)
        for switch_name in set(current_distribution.keys() + distribution.keys()):
            max_distribution[switch_name] = max(
                current_distribution.get(switch_name, 0),
                distribution.get(switch_name, 0)
            )

        return max_distribution

    sum_guarantees = {
        'pu': 0,
        'memory': 0,
        'hdd': 0,
        'ssd': 0,
        'net': 0,
        'max_per_switch': 10 ** 10,
        'switch_distribution': collections.defaultdict(int),
        'dc': {'man', 'sas', 'vla', 'iva', 'myt'}
    }
    for group in groups:
        locations = set([x.split('_')[-1] for x in map(str.lower, group.card.reqs.hosts.location.location)])
        locations = locations or {'man', 'sas', 'vla', 'iva', 'myt'}

        sum_guarantees['pu'] += group_power(group)
        sum_guarantees['memory'] += group.card.reqs.instances.memory_guarantee.gigabytes()
        sum_guarantees['hdd'] += group.card.reqs.instances.disk.gigabytes()
        sum_guarantees['ssd'] += group.card.reqs.instances.ssd.gigabytes()
        sum_guarantees['net'] += group.card.reqs.instances.net_guarantee.gigabytes()
        sum_guarantees['max_per_switch'] = min(
            sum_guarantees['max_per_switch'],
            group.card.reqs.hosts.max_per_switch or 10 ** 10
        )
        sum_guarantees['switch_distribution'] = max_switch_distribution(group, sum_guarantees['switch_distribution'])
        sum_guarantees['dc'] &= locations or sum_guarantees['dc']
    return sum_guarantees


def _filter_excluded_hosts(available_hosts, exclude_hosts):
    new_all_group_hosts = {x for x in available_hosts if x.name not in (exclude_hosts or [])}
    logging.info('Filtered {}/{} hosts by excluded hosts'.format(len(available_hosts) - len(new_all_group_hosts), len(available_hosts)))
    return new_all_group_hosts


def _filter_exclude_groups(available_hosts, exclude_groups):
    all_excluded_group_hosts = reduce(lambda x, y: x | y, (set(group.getHosts()) for group in (exclude_groups or [])), set())
    new_all_group_hosts = {x for x in available_hosts if x not in all_excluded_group_hosts}
    logging.info('Filtered {}/{} hosts by excluded groups'.format(len(available_hosts) - len(new_all_group_hosts), len(available_hosts)))
    return new_all_group_hosts


def _filter_wrong_host_params(available_hosts, pu=None, memory=None, hdd=None, ssd=None, net=None, dc=None):
    copy_all_group_hosts = copy.copy(available_hosts)

    hosts_params_filters = {
        'pu': lambda host: host.power >= (pu or 0),
        'memory': lambda host: host.memory >= (memory or 0),
        'hdd': lambda host: host.disk >= (hdd or 0),
        'ssd': lambda host: host.ssd >= (ssd or 0),
        'net': lambda host: host.net >= (net or 0),
        'dc': lambda host: host.dc in (dc or [])
    }
    for param_name, param_filter in hosts_params_filters.items():
        new_all_group_hosts = filter(param_filter, copy_all_group_hosts)
        logging.info('Filtered {}/{} hosts by wrong {}'.format(len(copy_all_group_hosts) - len(new_all_group_hosts), len(copy_all_group_hosts), param_name))
        copy_all_group_hosts = new_all_group_hosts

    return copy_all_group_hosts


def _filter_wrong_host_filter(available_hosts, hosts_filter=lambda host: True):
    new_all_group_hosts = filter(hosts_filter, available_hosts)
    logging.info('Filtered {}/{} hosts by wrong hosts filter'.format(len(available_hosts) - len(new_all_group_hosts), len(available_hosts)))
    return new_all_group_hosts


def _filter_wrong_host_free_resources(available_hosts, free_pu=None, free_memory=None, free_hdd=None, free_ssd=None, free_net=None):
    copy_all_group_hosts = copy.copy(available_hosts)

    hosts_free_resources_filters = {
        'pu': lambda host: host['power'] >= (free_pu or 0),
        'memory': lambda host: host['memory'] >= (free_memory or 0),
        'hdd': lambda host: host['hdd'] >= (free_hdd or 0),
        'ssd': lambda host: host['ssd'] >= (free_ssd or 0),
        'net': lambda host: host['net'] >= (free_net or 0)
    }
    for param_name, param_filter in hosts_free_resources_filters.items():
        all_group_hosts_free = [host_free_resources(x) for x in copy_all_group_hosts]
        new_all_group_hosts = {x['host'] for x in filter(param_filter, all_group_hosts_free)}
        logging.info('Filtered {}/{} hosts by wrong FREE {}'.format(len(copy_all_group_hosts) - len(new_all_group_hosts), len(copy_all_group_hosts), param_name))
        copy_all_group_hosts = new_all_group_hosts

    return copy_all_group_hosts


def _sorting_host_candidates(available_hosts, sort_by_free=False, sort_order=None):
    sort_order = sort_order or get_list_form_comma_list(DEFAULT_ORDERING)

    if sort_by_free:
        resources_group_hosts = [host_free_resources(x) for x in available_hosts]
    else:
        resources_group_hosts = [{'host': x, 'power': x.power, 'memory': x.memory, 'hdd': x.disk, 'ssd': x.ssd, 'net': x.net} for x in available_hosts]
    resources_group_hosts = sorted(resources_group_hosts, key=lambda x: tuple(x[y] for y in sort_order))

    return [x['host'] for x in resources_group_hosts]


def _filter_wrong_hosts_per_switch(available_hosts, hosts_per_switch=None, switch_distribution=None):
    switches = switch_distribution or collections.defaultdict(int)
    new_all_group_hosts = []
    for host in available_hosts:
        if switches[host.switch] >= (hosts_per_switch or 10 ** 10):
            continue
        new_all_group_hosts.append(host)
        switches[host.switch] += 1
    logging.info('Filtered {}/{} hosts by hosts per switch'.format(len(available_hosts) - len(new_all_group_hosts), len(available_hosts)))
    return new_all_group_hosts


def _filter_wrong_hosts_per_queue(available_hosts, hosts_per_queue=None):
    queues = collections.defaultdict(int)
    new_all_group_hosts = []
    for host in available_hosts:
        queues[host.queue] += 1
        if queues[host.queue] > (hosts_per_queue or 10 ** 10):
            continue
        new_all_group_hosts.append(host)
    logging.info('Filtered {}/{} hosts by hosts per queue'.format(len(available_hosts) - len(new_all_group_hosts), len(available_hosts)))
    return new_all_group_hosts


def _verbose_host_info(available_hosts):
    resources_selected_hosts = [host_free_resources(x) for x in available_hosts]

    logging.info('{:30}\t{:10}\t{:10}\t{:10}\t{:10}\t{:10}\t{:10}\t{:10}\t{:10}\t{:10}\t{:10}\t{:10}\t{:10}'.format(
        'hostname', 'free pu', 'pu', 'free memory', 'memory', 'free hdd', 'hdd', 'free ssd', 'ssd', 'free net', 'net', 'switch', 'queue'
    ))
    for res in resources_selected_hosts:
        logging.info('{:30}\t{:10.2f}\t{:10.2f}\t{:10.2f}\t{:10.2f}\t{:10.2f}\t{:10.2f}\t{:10.2f}\t{:10.2f}\t{:10.2f}\t{:10.2f}\t{:10}\t{:10}'.format(
            res['host'].name, res['power'], res['host'].power, res['memory'], res['host'].memory,
            res['hdd'], res['host'].disk, res['ssd'], res['host'].ssd, res['net'], res['host'].net,
            res['host'].switch, res['host'].queue
        ))
    logging.info('\n')


def find_hosts_for_groups(candidate_hosts, groups, count,
                          pu=None, memory=None, hdd=None, ssd=None, net=None, dc=None,
                          exclude_hosts=None, exclude_groups=None, hosts_filter=lambda host: True,
                          sort_by_free=False, sort_order=None):

    sum_guarantees = get_groups_sum_guarantees(groups)
    free_pu = sum_guarantees['pu']
    free_memory = sum_guarantees['memory']
    free_hdd = sum_guarantees['hdd']
    free_ssd = sum_guarantees['ssd']
    free_net = sum_guarantees['net']
    dc = set(dc) & set(sum_guarantees['dc'])
    hosts_per_switch = sum_guarantees['max_per_switch']
    switch_distribution = sum_guarantees['switch_distribution']

    return find_hosts(
        candidate_hosts=candidate_hosts,
        count=count,
        pu=pu,
        memory=memory,
        hdd=hdd,
        ssd=ssd,
        net=net,
        dc=dc,
        free_pu=free_pu,
        free_memory=free_memory,
        free_hdd=free_hdd,
        free_ssd=free_ssd,
        free_net=free_net,
        hosts_per_switch=hosts_per_switch,
        switch_distribution=switch_distribution,
        exclude_hosts=exclude_hosts,
        exclude_groups=exclude_groups,
        hosts_filter=hosts_filter,
        sort_by_free=sort_by_free,
        sort_order=sort_order,
    )


def find_hosts(candidate_hosts, count,
               pu=None, memory=None, hdd=None, ssd=None, net=None, dc=None,
               free_pu=None, free_memory=None, free_hdd=None, free_ssd=None, free_net=None,
               hosts_per_switch=None, hosts_per_queue=None, switch_distribution=None,
               exclude_hosts=None, exclude_groups=None, hosts_filter=lambda host: True,
               sort_by_free=False, sort_order=None):

    logging.info('Find hosts for: free_pu={}; free_memory={}; free_hdd={}; free_ssd={}; free_net={}; dc={}; hosts_per_switch={};'.format(
        free_pu, free_memory, free_hdd, free_ssd, free_net, ','.join(dc), hosts_per_switch
    ))

    available_hosts = candidate_hosts

    # Step 1. Remove excluded hosts
    available_hosts = _filter_excluded_hosts(available_hosts, exclude_hosts=exclude_hosts)

    # Step 2. Remove excluded groups
    available_hosts = _filter_exclude_groups(available_hosts, exclude_groups=exclude_groups)

    # Step 3. Remove wrong params
    available_hosts = _filter_wrong_host_params(available_hosts, pu=pu, memory=memory, hdd=hdd,
                                                ssd=ssd, net=net, dc=dc)

    # Step 4. Remove wrong filter
    available_hosts = _filter_wrong_host_filter(available_hosts, hosts_filter=hosts_filter)

    # Step 5. Remove wrong free resources
    available_hosts = _filter_wrong_host_free_resources(available_hosts, free_pu=free_pu, free_memory=free_memory,
                                                        free_hdd=free_hdd, free_ssd=free_ssd, free_net=free_net)

    # Step 6. Sorting
    available_hosts = _sorting_host_candidates(available_hosts, sort_by_free=sort_by_free, sort_order=sort_order)

    # Step 7. Remove wrong hosts per switch
    available_hosts = _filter_wrong_hosts_per_switch(available_hosts, hosts_per_switch=hosts_per_switch,
                                                     switch_distribution=switch_distribution)

    # Step 8. Remove wrong hosts per queue
    available_hosts = _filter_wrong_hosts_per_queue(available_hosts, hosts_per_queue=hosts_per_queue)

    # Cut count hosts from begin
    selected_hosts = available_hosts[:count] if count else available_hosts

    # Prepare verbose selected host info
    logging.info('\nSelected hosts:')
    _verbose_host_info(selected_hosts)

    return selected_hosts


def parse_cmd():
    parser = ArgumentParser(description='Script to search for hosts by parameters')

    parser.add_argument('-g', '--groups', type=argparse_types.groups,
                        default=DEFAULT_SRC_GROUP, help='Names of groups')
    parser.add_argument('-s', '--hosts', type=argparse_types.hosts, default=None, help='Source hosts')
    parser.add_argument('-c', '--count', type=int, help='Count hosts for search')

    parser.add_argument('--for-groups', type=argparse_types.groups,
                        default=None, help='Optional. Find hosts for add to groups.')
    parser.add_argument('--for-donor-groups', type=argparse_types.group,
                        default=None, help='Optional. Find hosts for add to master and all his donors.')
    parser.add_argument('--for-slaves-groups', type=argparse_types.group,
                        default=None, help='Optional. Find hosts for add to master and all his slaves.')

    parser.add_argument('--pu', type=int, help='Optional. Host min power units')
    parser.add_argument('--memory', type=float, help='Optional. Host min memory (Gb)')
    parser.add_argument('--hdd', type=float, help='Optional. Host min HDD size (Gb)')
    parser.add_argument('--ssd', type=float, help='Optional. Host min SSD size (Gb)')
    parser.add_argument('--net', type=int, help='Optional. Host min net (Mbit)')
    parser.add_argument('--dc', type=get_list_form_comma_list,
                        default=DEFAULT_DC, help='Optional. Host dc (list)')

    parser.add_argument('--free-pu', type=int, help='Optional. Host min FREE power units')
    parser.add_argument('--free-memory', type=float, help='Optional. Host min FREE memory (Gb)')
    parser.add_argument('--free-hdd', type=float, help='Optional. Host min FREE HDD size (Gb)')
    parser.add_argument('--free-ssd', type=float, help='Optional. Host min FREE SSD size (Gb)')
    parser.add_argument('--free-net', type=int, help='Optional. Host min FREE net (Mbit)')

    parser.add_argument('--hosts-per-switch', type=int, help='Count hosts per switch')
    parser.add_argument('--hosts-per-queue', type=int, help='Count hosts per queue')

    parser.add_argument('--exclude-hosts', type=get_list_form_comma_list, help='Optional. Hosts will be excluded')
    parser.add_argument('--exclude-groups', type=argparse_types.groups, help='Optional. Groups hosts will be excluded')

    parser.add_argument('--filter', type=argparse_types.pythonlambda,
                        default=None, help="optional. Search host by filter.")

    parser.add_argument('--sort-by-free', action='store_true',
                        default=False, help='Optional. Sorting by free or absolute resources.')
    parser.add_argument('--sort-order', type=get_list_form_comma_list,
                        default=DEFAULT_ORDERING, help='Optional. Sorting order')

    parser.add_argument('-v', '--verbose', action='store_true',
                        default=False, help='Optional. Explain what is being done.')

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    options = parser.parse_args()

    options.dc = [x.lower().replace('msk_', '') for x in options.dc]
    options.hosts = options.hosts if options.hosts is not None else groups_hosts(options.groups)
    if options.for_donor_groups is not None:
        options.for_groups = [options.for_donor_groups]
    elif options.for_slaves_groups is not None:
        options.for_groups = [options.for_slaves_groups]

    return options


def main():
    options = parse_cmd()

    if options.verbose:
        logging.basicConfig(level=logging.INFO, format='%(message)s')
    else:
        logging.basicConfig(level=logging.ERROR, format='%(message)s')

    if options.for_groups:
        selected_hosts = find_hosts_for_groups(
            candidate_hosts=options.hosts,
            groups=options.for_groups,
            count=options.count,
            pu=options.pu,
            memory=options.memory,
            hdd=options.hdd,
            ssd=options.ssd,
            net=options.net,
            dc=options.dc,
            exclude_hosts=options.exclude_hosts,
            exclude_groups=options.exclude_groups,
            hosts_filter=options.filter,
            sort_by_free=options.sort_by_free,
            sort_order=options.sort_order,
        )
    else:
        selected_hosts = find_hosts(
            candidate_hosts=options.hosts,
            count=options.count,
            pu=options.pu,
            memory=options.memory,
            hdd=options.hdd,
            ssd=options.ssd,
            net=options.net,
            dc=options.dc,
            free_pu=options.free_pu,
            free_memory=options.free_memory,
            free_hdd=options.free_hdd,
            free_ssd=options.free_ssd,
            free_net=options.free_net,
            hosts_per_switch=options.hosts_per_switch,
            hosts_per_queue=options.hosts_per_queue,
            exclude_hosts=options.exclude_hosts,
            exclude_groups=options.exclude_groups,
            hosts_filter=options.filter,
            sort_by_free=options.sort_by_free,
            sort_order=options.sort_order
        )

    if len(selected_hosts) < options.count:
        raise RuntimeError('Only {}/{} hosts with required resources found'.format(
            len(selected_hosts), options.count
        ))

    sep = ',' if options.verbose else '\n'
    print(sep.join([x.name for x in selected_hosts]))

    return 0


if __name__ == '__main__':
    ret_value = main()
    sys.exit(ret_value)

