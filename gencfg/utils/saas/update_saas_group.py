#!/skynet/python/bin/python

import os
import sys
import logging
from collections import defaultdict

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import gencfg
import argparse
import json
from core.db import CURDB
from core.card.updater import CardUpdater
from core.argparse.parser import ArgumentParserExt
import core.argparse.types as argparse_types
from utils.common.manipulate_volumes import main_put as put_volumes
from utils.common.update_card import EStatuses
from utils.saas.collect_saas_hosts import get_host_info
from utils.saas.saas_host_info import SaaSHostInfo
from utils.saas.saas_byte_size import SaaSByteSize
from utils.saas.saas_sort import sort_and_filter_by_switch
from utils.saas.common import instances_per_host, get_group_instance_power, get_group_allocated_volume, NumberWithSign, number_with_sign, is_saas_group
from utils.saas.common import get_network_critical, get_noindexing, IGNORE_NETWORK_CRITICAL_LOCATIONS, DEFAULT_IO_LIMITS, ALL_GEO
from utils.saas.common import gencfg_io_limits_from_json, merge_volumes, get_allocated_volume
from utils.saas.common import VolumesOptions
from utils.saas.errors import NotEnoughHosts, ActionNotAllowed, InvalidGroup, GroupUpdateError
from utils.common.fix_cpu_guarantee import main as fix_cpu_guarantee


def get_parser():

    parser = ArgumentParserExt(
        description="Update SaaS group with target params")

    count_args_group = parser.add_mutually_exclusive_group()

    count_args_group.add_argument('-n', '--instance-number', type=number_with_sign, required=False, default=NumberWithSign('0'),
                        help='Optional. Target slot count')
    count_args_group.add_argument('-nh', '--host-number', type=number_with_sign, required=False, default=NumberWithSign('0'),
                        help='Optional. Target number of hosts')

    parser.add_argument('-g', '--groups', type=argparse_types.groups, default=None, required=True,
                        help='Obligatory. Comma separated list of groups')
    parser.add_argument('-c', '--slot-size-cpu', type=number_with_sign, required=False, default=NumberWithSign('0'),
                        help='Optional. Target slot CPU in Kimkim power units (1 core ~= 40 Pu). Overrides current group ipower')
    parser.add_argument('-m', '--slot-size-mem', type=number_with_sign, required=False, default=NumberWithSign('0'),
                        help='Optional. Target slot memery Gb. Overrides current group imemory')
    parser.add_argument('-sc', '--max-instances-per-switch', type=int, required=False,
                        help='Max number of instances in one switch')
    parser.add_argument('-nc', '--network-critical', choices=['true', 'false', 'none'], default='none',
                        help='Optional. true and false will change group network critical tag, none will use current value')
    parser.add_argument('-i', '--no-indexing', choices=['true', 'false', 'none'], default='none',
                        help='Optional. true and false will change group noindexing, none will use current value')
    parser.add_argument('-y', '--allow-host-change', action='store_true',
                        help='Optional. Allow host replacing. Ignore --allow-cpu-overcommit option.')
    parser.add_argument('-o', '--allow-cpu-overcommit', action='store_true',
                        help='Optional. Do not try to move existing instances from cpu starving hosts.')
    parser.add_argument('-oi', '--ignore-noindexing-mismatch', action='store_true',
                        help='Optional. Do not try to move existing instances from hosts with wrong noindexing status')
    parser.add_argument('-os', '--ignore-max-hosts-per-switch', action='store_true',
                        help='Ignore restriction on number of hosts per switch')
    parser.add_argument('-b', '--blacklisted-groups', type=argparse_types.groups,
                        help='Optional. Don\'t use hosts from this groups')
    parser.add_argument('-cl', '--slot-cpu-limit', type=int, required=False, default=0,
                        help='Optional. Set group class_name to greedy and set this value as greedy_limit')
    parser.add_argument('-il', '--io-limits', type=json.loads, required=False, default=dict(),
                        help='IO limits json, ex: {}'.format(json.dumps(DEFAULT_IO_LIMITS)))
    parser.add_argument('-d', '--disk-volumes', type=json.loads, required=False, default=dict(),
                        help="""
                        Volumes Definitions in list or dict format. ex:
                        {"/db/bsconfig/webstate": {"quota": "550 Gb", "host_mp_root": "/ssd"}, "/ssd": {"host_mp_root": "/hdd"}} or
                        [{"guest_mp": "/db/bsconfig/webstate", "quota": "550 Gb", "host_mp_root": "/ssd"}, {"guest_mp": "/ssd", "host_mp_root": "/hdd", "symlinks": ["/slow_data"] }]
                        """
                        )
    parser.add_argument('-O', '--force-overcommit-check', action='store_true',
                        help='Check overcommit on hosts even if requirements not changed (and try to replace overcommited hosts)')
    parser.add_argument('-pe', '--prefer-empty', action='store_true',
                        help='Prefer hosts with more free resource')
    parser.add_argument('-op', '--allow-partial-update', action='store_true',
                        help='Do not fail overcommit check if not enough hosts to replace all overcommited and change as much as possible')
    parser.add_argument('-f', '--file', type=argparse.FileType('w'),
                        help='Changes in json format')
    parser.add_argument('-v', '--verbose', action='store_true',
                        help='Print hysterical messages')
    return parser


def host_overcommit_on_recluster(group, req_memory, req_cpu_pu, req_ssd, no_indexing, max_hosts_per_switch, network_critical,
                                 ignore_cpu_overcommit=False, force_check_overcommit=False,
                                 ignore_noindexing=False, blacklisted_groups=None):
    """
    :param group:
    :type group: core.igroups.IGroup
    :param req_memory: memory requirements per instance
    :type req_memory: SaaSByteSize
    :param req_cpu_pu: cpu requirements per instance
    :type req_cpu_pu: int
    :param req_ssd:
    :param no_indexing:
    :param ignore_cpu_overcommit:
    :param force_check_overcommit:
    :type blacklisted_groups: set[core.igroups.IGroup]
    :rtype: List[SaaSHostInfo]
    """
    hosts_to_replace = set()
    switch_hosts_count = defaultdict(list)
    current_instance_power = get_group_instance_power(group)
    current_group_ssd = get_group_allocated_volume(group, '/ssd')
    iph = instances_per_host(group)

    need_check = force_check_overcommit

    if group.card.reqs.instances.memory_guarantee < req_memory:
        logging.info('Group memory requirements changed')
        need_check = True
    if current_instance_power < req_cpu_pu:
        logging.info('Group cpu requirements changed')
        need_check = True
    if current_group_ssd < req_ssd:
        logging.info('Group ssd requirements changed')
        need_check = True
    if no_indexing != get_noindexing(group):
        logging.info('Group noindexing requirements changed')
        need_check = True
    if network_critical != get_network_critical(group):
        logging.info('Network affinity changed')
        need_check = True
    if max_hosts_per_switch < group.card.reqs.hosts.max_per_switch or (not group.card.reqs.hosts.max_per_switch and max_hosts_per_switch):
        logging.info('Group max hosts per switch requirements changed')
        need_check = True
    if blacklisted_groups:
        logging.info('Checking intersection with blacklisted groups')
        need_check = True
        blacklisted_groups = set(blacklisted_groups)
    else:
        blacklisted_groups = set()

    if need_check:
        hosts_info = map(lambda x: SaaSHostInfo(x), group.getHosts())
        for host in hosts_info:
            logging.debug('Checking host {}'.format(host))
            memory_added_per_host = (req_memory - group.card.reqs.instances.memory_guarantee) * iph
            cpu_added_per_host = (req_cpu_pu - current_instance_power) * iph
            ssd_added_per_host = (req_ssd - current_group_ssd) * iph
            host_needs_replacement = False

            if blacklisted_groups:
                blacklisted_groups_intersection = [g.card.name for g in host.groups.intersection(blacklisted_groups)]
            else:
                blacklisted_groups_intersection = False

            if host.memory_left < memory_added_per_host:
                logging.debug('Host %s has not enough memory for group: %s/%s', host.host.name, host.memory_left, memory_added_per_host)
                host_needs_replacement = True
            if host.cpu_left < cpu_added_per_host and not ignore_cpu_overcommit:
                logging.debug('Host %s has not enough cpu for group: %s/%s', host.host.name, host.cpu_left, cpu_added_per_host)
                host_needs_replacement = True
            if host.ssd_left < ssd_added_per_host:
                logging.debug('Host %s has not enough ssd for group: %s/%s', host.host.name, host.ssd_left, ssd_added_per_host)
                host_needs_replacement = True
            if host.no_indexing is not None and no_indexing is not None and host.no_indexing != no_indexing and not ignore_noindexing:
                logging.debug('Host %s is not suitable for group due to noindexing mismatch: %s/%s', host.host.name, host.no_indexing, no_indexing)
                host_needs_replacement = True
            if ((network_critical and host.net < 10000 and host.has_network_critical_groups([group])) or (not network_critical and host.net >= 10000)) \
                    and (host.host.dc.upper() not in IGNORE_NETWORK_CRITICAL_LOCATIONS):
                logging.debug('Host net %d not match requested network critical state %s', host.net, network_critical)
                host_needs_replacement = True
            if blacklisted_groups_intersection:
                logging.debug('Host %s present in blacklisted groups %s', host.host.name, ','.join(blacklisted_groups_intersection))
                host_needs_replacement = True

            if host_needs_replacement:
                hosts_to_replace.add(host)
            else:
                switch_hosts_count[host.switch].append(host)

        for sw, v in switch_hosts_count.iteritems():
            if max_hosts_per_switch and len(v) > max_hosts_per_switch:
                logging.info('Sort hosts in %s', sw)
                good_hosts = set(
                    sort_and_filter_by_switch(
                        v, req_memory, req_cpu_pu, req_ssd, network_critical, max_hosts_per_switch=max_hosts_per_switch
                    )
                )
                hosts_to_replace |= (set(v) - good_hosts)
    return hosts_to_replace


def get_hosts_for_partial_replace(group, hosts_pool, replacement_pool_size, req_memory, req_cpu_pu, req_ssd, network_critical, blacklisted_groups=None):
    must_be_replaced = set()
    should_replace = set()
    iph = instances_per_host(group)
    blacklisted_groups = blacklisted_groups if blacklisted_groups is not None else set()
    memory_added_per_host = (req_memory - group.card.reqs.instances.memory_guarantee) * iph
    ssd_added_per_host = (req_ssd - get_group_allocated_volume(group, '/ssd')) * iph
    cpu_added_per_host = (req_cpu_pu - get_group_instance_power(group)) * iph
    for host in hosts_pool:
        if host.groups.intersection(blacklisted_groups) or host.memory_left < memory_added_per_host or host.ssd_left < ssd_added_per_host:
            must_be_replaced.add(host)
            continue
        else:
            should_replace.add(host)
    if len(must_be_replaced) > replacement_pool_size:
        raise NotEnoughHosts(
            must_be_replaced, replacement_pool_size,
            'Only {} hosts available for replacement, while {} must be replaced'.format(replacement_pool_size, must_be_replaced)
        )
    else:
        should_replace = sort_and_filter_by_switch(list(should_replace), memory_added_per_host, cpu_added_per_host, ssd_added_per_host, network_critical)
        while len(must_be_replaced) < replacement_pool_size and should_replace:
            must_be_replaced.add(should_replace.pop())

    return must_be_replaced


def main(group, instance_count=0, req_memory=None, req_cpu=0, no_indexing=None, network_critical=None, max_instances_per_switch=0,
         ignore_cpu_overcommit=False, move_hosts=False, blacklisted_groups=None, force_check_overcommit=False, cpu_limit=0,
         ignore_noindexing_mismatch=False, ignore_max_hosts_per_switch=False, prefer_empty=False, allow_partial_update=False,
         io_limits=None, disk_volumes=None):
    """
    :param instance_count: target total instance count
    :type instance_count: int
    :type req_memory: SaaSByteSize
    :type req_cpu: int
    :type req_ssd: SaaSByteSize
    :type no_indexing: bool
    :type max_instances_per_switch: int
    :type ignore_cpu_overcommit: bool
    :type move_hosts: bool
    :type blacklisted_groups: List[str]
    :type force_check_overcommit: bool
    :type cpu_limit: int
    :type ignore_max_hosts_per_switch: bool
    :type ignore_noindexing_mismatch: bool
    :type prefer_empty: bool
    :return: Nothing
    """
    changes = {}
    master_group = group.card.master
    if not is_saas_group(group):
        logging.fatal('Invalid group. Only slave groups of (SAS|MAN|VLA)_SAAS_CLOUD supported')
        raise InvalidGroup('Only slave groups of ({})_SAAS_CLOUD supported'.format('|'.join(ALL_GEO)))

    iph = instances_per_host(group)

    # collect group requirements
    group_host_count = len(group.getHosts())
    group_instance_count = group_host_count * iph
    max_hosts_per_switch = max_instances_per_switch//iph if max_instances_per_switch else group.card.reqs.hosts.max_per_switch
    target_instance_count = instance_count if instance_count > 0 else group_instance_count
    blacklisted_groups = set(blacklisted_groups) if blacklisted_groups else set()
    network_critical = network_critical if network_critical is not None else get_network_critical(group)

    if ignore_max_hosts_per_switch:
        max_hosts_per_switch = 0

    group_memory = SaaSByteSize(group.card.reqs.instances.memory_guarantee)
    req_memory = req_memory if req_memory else group_memory
    if req_memory != group_memory:
        changes['memory'] = (req_memory - group_memory).value

    if disk_volumes:
        disk_volumes = merge_volumes(group.card.reqs.volumes, disk_volumes)
        req_ssd = get_allocated_volume(disk_volumes, '/ssd')
    else:
        req_ssd = get_group_allocated_volume(group, '/ssd')

    group_cpu = get_group_instance_power(group)

    req_cpu = req_cpu if req_cpu > 0 else group_cpu
    if req_cpu != group_cpu:
        changes['cpu'] = req_cpu - group_cpu

    cpu_limit = cpu_limit or group.card.audit.cpu.greedy_limit or req_cpu + 40

    if cpu_limit < req_cpu:
        cpu_limit = req_cpu + 40
        logging.warning('CPU limit lesser than guarantee. Updating limit to %d Pu', cpu_limit)

    group_noindexing = get_noindexing(group)
    no_indexing = no_indexing if no_indexing is not None else group_noindexing
    if no_indexing != group_noindexing:
        changes['noindexing'] = no_indexing

    logging.info(
        'GROUP REQUIREMENTS: cpu:{}; cpu_limit: {}; mem:{}; ssd:{}; noindex:{}; net crit:{}, per host:{}; per switch:{}; total:{}'.format(
            req_cpu, cpu_limit, req_memory, req_ssd, no_indexing, network_critical, iph, max_hosts_per_switch * iph, target_instance_count
        )
    )

    if 0 < target_instance_count < group_instance_count and not move_hosts:
        print('Please set --allow-host-change to remove instances from group')
        raise ActionNotAllowed('Host change required but not allowed')
    if (target_instance_count - group_instance_count) % iph != 0:
        raise ValueError('Can\'t place {} instances with {} instances per host. Please change instance function'.format(
            target_instance_count, iph
        ))
    # move instances from cpu overcommited hosts if allowed
    # ignore_cpu_overcommit = ignore_cpu_overcommit if not move_hosts else False

    hosts_to_replace = host_overcommit_on_recluster(
        group, req_memory, req_cpu, req_ssd, no_indexing, max_hosts_per_switch, network_critical, ignore_cpu_overcommit=ignore_cpu_overcommit,
        force_check_overcommit=force_check_overcommit, ignore_noindexing=ignore_noindexing_mismatch, blacklisted_groups=blacklisted_groups
    )
    blacklisted_groups_for_new_hosts = blacklisted_groups.copy()
    blacklisted_groups_for_new_hosts.add(group)
    new_hosts = get_host_info(master_group, req_memory * iph, req_cpu * iph, req_ssd * iph, no_indexing, network_critical,
                              exclude_groups=blacklisted_groups_for_new_hosts, max_hosts_per_switch=max_hosts_per_switch, target_group=group,
                              prefer_empty=prefer_empty, strict=True)
    # remove problem hosts
    total_bad_hosts_number = len(hosts_to_replace)
    available_hosts_count = len(new_hosts)
    can_use_for_replacement = available_hosts_count - ((target_instance_count - group_instance_count) // iph)

    logging.warning('%d hosts need replacement', total_bad_hosts_number)
    logging.warning('%d hosts available for replacement', can_use_for_replacement)

    if total_bad_hosts_number and move_hosts:
        if total_bad_hosts_number > can_use_for_replacement:
            if allow_partial_update:
                logging.warning('Will perform partial replace (%d/%d)', can_use_for_replacement, total_bad_hosts_number)
                hosts_to_replace = get_hosts_for_partial_replace(group, hosts_to_replace, can_use_for_replacement,
                                                                 req_memory, req_cpu, req_ssd, network_critical,
                                                                 blacklisted_groups=blacklisted_groups)
                logging.info('Hosts to replace: %s', hosts_to_replace)
            else:
                raise NotEnoughHosts(
                    total_bad_hosts_number, can_use_for_replacement,
                    'Only {} hosts available for replacement, while {} should be replaced'.format(can_use_for_replacement, total_bad_hosts_number)
                )

        CURDB.groups.remove_slave_hosts(map(lambda hh: getattr(hh, 'host'), hosts_to_replace), group)
        changes['removed_hosts'] = [h.host.name for h in hosts_to_replace]
    elif hosts_to_replace and not move_hosts:
        raise ActionNotAllowed('Can\'t satisfy required resources without changing hosts. Problem hosts: {}'.format(
            ','.join([h.host.name for h in hosts_to_replace]))
        )
    else:
        pass  # No hosts to remove

    # update group requirements
    itags = group.card.tags.itag
    if no_indexing and ('saas_no_indexing' not in group.card.tags.itag):
        itags.append('saas_no_indexing')
    elif no_indexing is False and ('saas_no_indexing' in group.card.tags.itag):
        itags = [tag for tag in group.card.tags.itag if tag != 'saas_no_indexing']

    if network_critical and ('saas_network_critical' not in group.card.tags.itag):
        itags.append('saas_network_critical')
    elif network_critical is False and ('saas_network_critical' in group.card.tags.itag):
        itags = [tag for tag in group.card.tags.itag if tag != 'saas_network_critical']

    updates = {
        ('reqs', 'instances', 'memory_guarantee'): req_memory.__repr__('Gb'),
        ('reqs', 'instances', 'affinity_category'): 'saas_no_indexing' if no_indexing else group.card.reqs.instances.affinity_category,
        ('reqs', 'hosts', 'max_per_switch'): max_hosts_per_switch if max_instances_per_switch and not ignore_max_hosts_per_switch else group.card.reqs.hosts.max_per_switch,
        ('tags', 'itag'): itags
    }

    if cpu_limit > 0:
        updates[('audit', 'cpu', 'class_name')] = 'greedy'
        updates[('audit', 'cpu', 'greedy_limit')] = cpu_limit

    if io_limits:
        updates.update(gencfg_io_limits_from_json(io_limits))

    logging.info('UPDATES FOR GROUP: {}'.format(updates))

    has_updates, is_ok, invalid_values = CardUpdater().update_group_card(group, updates, mode="util")

    options = type('Options', (object,), {'groups': [group], 'power': req_cpu})()
    fix_cpu_guarantee(options)

    if not is_ok:
        logging.error('Gencfg update failed: %s', invalid_values.values())
        raise RuntimeError('{},{}'.format(EStatuses.STATUS_FAIL, '\n'.join(invalid_values.values())))
    else:
        group.modified = True
        if disk_volumes:
            for dv in disk_volumes:
                dv['quota'] = str(dv['quota'])
            volumes_update = VolumesOptions([group.card.name], disk_volumes)
            put_volumes(volumes_update)


    # manipulate instance count
    logging.info('Old host count: %d', group_host_count)
    still_hosts = group_host_count - len(hosts_to_replace)
    logging.debug('Still hosts count: %d', still_hosts)
    logging.info('Target host count: %d', target_instance_count/iph)
    new_host_count = (target_instance_count - still_hosts * iph)/iph
    logging.debug('New hosts count: %d', new_host_count)

    if new_host_count > 0:
        logging.info('Adding %d new hosts', new_host_count)

        if available_hosts_count < new_host_count:
            raise NotEnoughHosts(
                new_host_count, available_hosts_count,
                '{} new hosts required, only {} found'.format(new_host_count, available_hosts_count)
            )

        hosts_to_add = new_hosts[:new_host_count]
        changes['added_hosts'] = [h.host.name for h in hosts_to_add]
        logging.info('Hosts to add: {}'.format(hosts_to_add))
        CURDB.groups.add_slave_hosts(map(lambda h: getattr(h, 'host'), hosts_to_add), group)
    elif new_host_count < 0:
        # remove redundant hosts
        logging.warning('Removing {} extra hosts'.format(abs(new_host_count)))
        hosts = map(lambda x: SaaSHostInfo(x), group.getHosts())
        sorted_hosts = sort_and_filter_by_switch(
            hosts, req_memory, req_cpu, req_ssd, network_critical, max_hosts_per_switch=len(hosts)
        )
        hosts_to_remove = sorted_hosts[new_host_count:]
        if move_hosts:
            CURDB.groups.remove_slave_hosts(map(lambda h: getattr(h, 'host'), hosts_to_remove), group)
        else:
            print('Please set --allow-host-change to remove instances from group')
            raise ActionNotAllowed('Host remove not allowed')
    CURDB.update(smart=True)
    print('Success')
    return changes


if __name__ == '__main__':
    args = get_parser().parse_cmd()
    success = True
    if args.verbose:
        log_level = logging.DEBUG
    else:
        log_level = logging.INFO
    logging.basicConfig(level=log_level)

    full_diff = {}
    if args.no_indexing == 'true':
        no_indexing = True
    elif args.no_indexing == 'false':
        no_indexing = False
    else:
        no_indexing = None

    if args.network_critical == 'true':
        network_critical = True
    elif args.network_critical == 'false':
        network_critical = False
    else:
        network_critical = None

    for group in args.groups:
        logging.info('Processing {}'.format(group))
        iph = instances_per_host(group)
        group_host_count = len(group.getHosts())
        group_instance_count = group_host_count * iph

        instance_number = args.instance_number or (args.host_number * iph)

        if args.slot_size_cpu.sign == '=':
            cpu_guarantee = args.slot_size_cpu.value
        else:
            cpu_guarantee = get_group_instance_power(group) + args.slot_size_cpu.value

        if args.slot_size_mem.sign == '=':
            req_mem = SaaSByteSize('{} Gb'.format(args.slot_size_mem.value)) if args.slot_size_mem.value > 0 else None
        else:
            mem_diff = SaaSByteSize('{} Gb'.format(abs(args.slot_size_mem.value)))
            group_mem = SaaSByteSize(group.card.reqs.instances.memory_guarantee)
            if args.slot_size_mem.sign == '-':
                req_mem = group_mem - mem_diff
                assert req_mem.value > 0, 'Result group memory lesser or equal to 0'
            else:
                req_mem = group_mem + mem_diff

        if instance_number.sign == '=':
            target_instance_count = instance_number.value
        else:
            target_instance_count = group_instance_count + instance_number.value
            if target_instance_count < 0:
                raise RuntimeError('Negative instance count not allowed')
        try:
            group_diff = main(
                group, instance_count=target_instance_count, req_memory=req_mem, req_cpu=cpu_guarantee,
                no_indexing=no_indexing, network_critical=network_critical,
                ignore_cpu_overcommit=args.allow_cpu_overcommit, move_hosts=args.allow_host_change, max_instances_per_switch=args.max_instances_per_switch,
                blacklisted_groups=args.blacklisted_groups, cpu_limit=args.slot_cpu_limit, force_check_overcommit=args.force_overcommit_check,
                ignore_noindexing_mismatch=args.ignore_noindexing_mismatch,ignore_max_hosts_per_switch=args.ignore_max_hosts_per_switch,
                prefer_empty=args.prefer_empty, allow_partial_update=args.allow_partial_update, io_limits=args.io_limits, disk_volumes=args.disk_volumes
            )
            if group_diff:
                full_diff[group.card.name] = group_diff
        except NotEnoughHosts as e:
            success = False
            logging.fatal(e)
            full_diff[group.card.name] = {'error': 'NotEnoughHosts', 'required': e.required_hosts, 'available': e.available_hosts, 'msg': e.message}
        except InvalidGroup as e:
            success = False
            logging.fatal(e)
            full_diff[group.card.name] = {'error': 'InvalidGroup', 'msg': e.message}
        except GroupUpdateError as e:
            success = False
            logging.fatal(e)
            full_diff[group.card.name] = {'error': 'GroupUpdateError', 'msg': e.message}

    if args.file:
        json.dump(full_diff, args.file)

    if not success:
        logging.fatal('Group update failed')
        exit(1)
