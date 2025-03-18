#!/skynet/python/bin/python
"""Script to amdin ipv4tunnels on ipv6-only machines (GENCFG-1077)"""

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import json
import urllib2
import time

import gencfg
from core.db import CURDB
from core.argparse.parser import ArgumentParserExt
from core.settings import SETTINGS
from core.ipv4tunnels import TIpv4Tunnels

import netaddr

import gaux.aux_utils
import gaux.aux_hbf


class EActions(object):
    """All action to perform on ipv4 tunnels"""
    STATS = 'stats'  # show stats of local base
    DIFF = 'diff'  # show diff with racktables
    EXPORT = 'export'  # export gencfg data to racktables
    UPDATE = 'update'
    CLEANUP = 'cleanup'  # cleanup database from tunnel ips, waiting for removal (RX-526)
    FREE = 'free'  # cleanup database from tunnel ips, without check expire
    REPLACE = 'replace'
    DUMP_REMOTE = 'dump_remote'  # dump remote (racktables) database
    # pool modifications
    ADD_POOL = 'add_pool'  # add new pool
    ADD_POOL_SUBNET = 'add_pool_subnet'
    SHOW_AFFECTED = 'show_affected'
    SHOW_SUBNETS = 'show_subnets'

    ALL = [STATS, DIFF, EXPORT, UPDATE, CLEANUP, FREE, DUMP_REMOTE, ADD_POOL, ADD_POOL_SUBNET, SHOW_AFFECTED, REPLACE,
           SHOW_SUBNETS]


def get_parser():
    parser = ArgumentParserExt(description='Manipulate on ipv4tunnels of on ipv6-only machines')
    parser.add_argument('-a', '--action', type=str, default = EActions.STATS,
                        choices=EActions.ALL,
                        help='Obligatory. Action to execute')
    parser.add_argument('-v', '--verbose-level', action='count', default=0,
                        help='Optional. Verbose level (maximum is 1)')
    parser.add_argument('-p', '--pool-name', type=str, default=None,
                        help='Optional. Pool name for some actions ({})'.format(EActions.ADD_POOL))
    # pool subnet
    parser.add_argument('--subnet-descr', type=str, default=None,
                        help='Optional. Subnet descr (used in action {})'.format(EActions.ADD_POOL_SUBNET))
    parser.add_argument('--subnet-filter', type=str, default=None,
                        help='Optional. Subnet filter (e. g. <lambda x: x.host.dc == "myt"> to be applied only for myt machines). Used in action {}'.format(EActions.ADD_POOL_SUBNET))
    parser.add_argument('--subnet-mask', type=str, default=None,
                        help='Optional. Subnet mask for xubnet (e. g. <95.108.129.0/26>. Used in action {}'.format(EActions.ADD_POOL_SUBNET))
    parser.add_argument('--subnet-masks', type=str, default=None,
                        help='Optional. Comma-separeted subnet masks')
    parser.add_argument('--dry-run', type=bool, default=False,
                        help='Optional. Only for export, try operation but don\'t make changes')
    parser.add_argument('--token', type=str, default="",
                        help='Token for racktables')
    return parser


def normalize(options):
    if options.action == EActions.ADD_POOL:
        if options.pool_name is None:
            raise Exception('You must specify <--pool-name> option with action <{}>'.format(options.action))

    if options.action == EActions.ADD_POOL_SUBNET:
        if options.pool_name is None:
            raise Exception('You must specify <--pool-name> option with action <{}>'.format(options.action))
        if options.subnet_descr is None:
            raise Exception('You must specify <--subnet-descr> options with action <{}>'.format(options.action))
        if options.subnet_filter is None:
            raise Exception('You must specify <--subnet-filter> options with action <{}>'.format(options.action))
        if options.subnet_mask is None:
            raise Exception('You must specify <--subnet-mask> options with action <{}>'.format(options.action))

    if options.action == EActions.SHOW_AFFECTED and options.subnet_masks is None:
        raise Exception('You must specify <--subnet-masks> options with action <{}>'.format(options.action))


def masks_to_ip_list(subnet_masks):
    all_ips = set()
    for subnet_mask in subnet_masks:
        new_subnet_mask = netaddr.IPNetwork(subnet_mask)
        all_ips |= set(map(str, list(new_subnet_mask)))
    return all_ips


def get_affected_instances(affected_ips):
    affected_instances = {}
    for allocated, ipv4 in CURDB.ipv4tunnels.allocated.items():
        if ipv4 in affected_ips:
            affected_instances[(allocated[0], allocated[1])] = ipv4
    return affected_instances


def main_stats(options):
    """Print current statistics"""
    result = str(CURDB.ipv4tunnels)
    if options.verbose_level > 0:
        result = '{}\n{}'.format(result, CURDB.ipv4tunnels.as_str_extended())

    return result


def main_diff(options):
    """Compare local state with racktables"""
    # get racktables ips
    url = SETTINGS.services.racktables.ip4tunnel.rest.geturl
    racktables_as_json = json.loads(gaux.aux_utils.retry_urlopen(3, url))
    racktables_ips = {x:y['fqdn'] for x, y in racktables_as_json.iteritems() if CURDB.ipv4tunnels.ip_in_mask_list(x)}

    # get our ips
    gencfg_ips = {x:CURDB.ipv4tunnels.get_instance_hbf_info_by_ip(x)['interfaces']['backbone']['ipv6addr'] for x in set(CURDB.ipv4tunnels.allocated.values())}
    common_ips = []
    different_fqdn_ips = []
    for ip in set(racktables_ips.keys()) & set(gencfg_ips.keys()):
        if racktables_ips[ip] == gencfg_ips[ip]:
            common_ips.append(ip)
        else:
            different_fqdn_ips.append(ip)

    # show diff
    extra_gencfg_ips = set(gencfg_ips.keys()) - set(racktables_ips.keys())
    gencfg_ips.update({x['tunnel_ip']:None for x in CURDB.ipv4tunnels.waiting_removal})
    extra_racktables_ips = set(racktables_ips.keys()) - set(gencfg_ips.keys())

    return set(common_ips), set(different_fqdn_ips), extra_racktables_ips, extra_gencfg_ips


def main_dump_remote(options):
    """Dump info, already stored in racktables"""
    url = SETTINGS.services.racktables.ip4tunnel.rest.geturl
    racktables_as_json = json.loads(gaux.aux_utils.retry_urlopen(3, url))
    racktables_as_json = {x: y for x, y in racktables_as_json.iteritems() if CURDB.ipv4tunnels.ip_in_mask_list(x)}

    result = []
    for ip in sorted(racktables_as_json):
        result.append('{}'.format(ip))
        for k in sorted(racktables_as_json[ip]):
            result.append('   {}: {}'.format(k, racktables_as_json[ip][k]))
    result = '\n'.join(result)

    return result


def main_export(options):
    """Export gencfg tunnels info to racktables"""
    common_ips, different_fqdn_ips, extra_racktables_ips, extra_gencfg_ips = main_diff(options)

    if options.token == "":
        raise Exception("No token")

    for ip in extra_racktables_ips | different_fqdn_ips:
        url = SETTINGS.services.racktables.ip4tunnel.rest.updateurl
        data = json.dumps(dict(addr=ip))
        headers = {
            'Content-Type': 'application/json',
            'Auth-Token': options.token,
        }

        req = urllib2.Request(url, data, headers)
        req.get_method = lambda: 'DELETE'
        if not options.dry_run:
            try:
                resp = urllib2.urlopen(req).read()
            except Exception, e:
                raise Exception("Got exception while updating racktables ipv4tunnels info with url <{}> and data <{}>: {}".format(url, data, str(e)))
        else:
            print('Delete request:', req.data)
    for ip in extra_gencfg_ips | different_fqdn_ips:
        hostname = CURDB.ipv4tunnels.get_host_by_ip(ip)
        url = SETTINGS.services.racktables.ip4tunnel.rest.updateurl
        data = json.dumps({
            'fqdn': CURDB.ipv4tunnels.get_instance_hbf_info_by_ip(ip)['interfaces']['backbone']['ipv6addr'],
            'addr': ip,
        })
        headers = {
            'Content-Type': 'application/json',
            'Auth-Token': options.token,
        }

        req = urllib2.Request(url, data, headers)
        if not options.dry_run:
            try:
                urllib2.urlopen(req).read()
            except urllib2.HTTPError as e:
                raise Exception("Updating racktables ipv4tunnels info with url <{}> and data <{}>\n{}: {}".format(
                    url, data, str(e), e.read()
                ))
            except Exception as e:
                raise Exception("Updating racktables ipv4tunnels info with url <{}> and data <{}>: {}".format(
                    url, data, str(e)
                ))
        else:
            print('Update request:', req.data)

    result = ['Added {} ips to racktables: {}'.format(len(extra_gencfg_ips), ' '.join(extra_gencfg_ips)),
              'Replaced {} ips to racktables: {}'.format(len(different_fqdn_ips), ' '.join(different_fqdn_ips)),
              'Remove {} ips from racktables: {}'.format(len(extra_racktables_ips), ' '.join(extra_racktables_ips))]

    return '\n'.join(result)


def main_add_pool(options):
    if options.pool_name in CURDB.ipv4tunnels.pool.keys():
        raise Exception('Pool <{}> already exists'.format(options.pool_name))

    CURDB.ipv4tunnels.pool[options.pool_name] = TIpv4Tunnels.TIpv4Pool([])

    CURDB.ipv4tunnels.update()


def main_add_pool_subnet(options):
    if options.pool_name not in CURDB.ipv4tunnels.pool.keys():
        raise Exception('Pool <{}> does not exists'.format(options.pool_name))

    # validate subnet_descr for uniquness
    subnet_descrs = {x.descr for x in CURDB.ipv4tunnels.get_all_subnets()}
    if options.subnet_descr in subnet_descrs:
        raise Exception('Subnet descr <{}> is not unique'.format(options.subnet_descr))

    # check filter func to be valid lambda
    try:
        eval(options.subnet_filter)
    except Exception as e:
        raise Exception('Can not parse subnet filter <{}>: got exception {}'.format(options.subnet_filter, str(e)))

    # check for valid range in subnet
    new_subnet_mask = netaddr.IPNetwork(options.subnet_mask)
    #for subnet in CURDB.ipv4tunnels.get_all_subnets():
    #    if new_subnet_mask.overlaps(subnet.subnet) or subnet.subnet.overlaps(new_subnet_mask):
    #        raise Exception('New subnet <{}> overlaps with <{}>'.format(new_subnet_mask, subnet.subnet))

    # create new subnet
    free_ips = list(new_subnet_mask)

    new_subnet = TIpv4Tunnels.TIpv4Pool.TIpv4LocPool(subnet=options.subnet_mask, descr=options.subnet_descr,
                                                     flt=options.subnet_filter, free_ips=free_ips)
    CURDB.ipv4tunnels.pool[options.pool_name].loc_pools.append(new_subnet)

    CURDB.ipv4tunnels.update()


def main_update_pool_subnet(options):
    all_existing_ips = []
    all_subnet_ips = {}

    for subnet in CURDB.ipv4tunnels.get_all_subnets():
        all_existing_ips += map(str, subnet.free_ips)
        for ip in map(str, list(subnet.subnet)):
            all_subnet_ips[ip] = subnet
    all_existing_ips += map(str, CURDB.ipv4tunnels.allocated.values())
    all_existing_ips += [str(x['tunnel_ip']) for x in CURDB.ipv4tunnels.waiting_removal]

    new_ips = set(all_subnet_ips.keys()).difference(set(all_existing_ips))

    print('Adding')
    for ip in new_ips:
        subnet = all_subnet_ips[ip]
        subnet.free_ips.append(netaddr.IPAddress(ip))
        print('    {} -> {}'.format(ip, str(subnet.subnet)))

    CURDB.ipv4tunnels.update()


def main_cleanup(options):
    now = int(time.time())

    remove_ips = [x['tunnel_ip'] for x in CURDB.ipv4tunnels.waiting_removal if x['expired_at'] <= now]
    for remove_ip in remove_ips:
        CURDB.ipv4tunnels.get_pool_by_ip(remove_ip).free(remove_ip)
    CURDB.ipv4tunnels.waiting_removal = [x for x in CURDB.ipv4tunnels.waiting_removal if x['expired_at'] > now]

    CURDB.ipv4tunnels.update()


def main_free(options):
    subnet_ips = map(str, list(netaddr.IPNetwork(options.subnet_mask)))

    remove_ips = [x['tunnel_ip'] for x in CURDB.ipv4tunnels.waiting_removal if x['tunnel_ip'] in subnet_ips]
    for remove_ip in remove_ips:
        CURDB.ipv4tunnels.get_pool_by_ip(remove_ip).free(remove_ip)
    CURDB.ipv4tunnels.waiting_removal = [x for x in CURDB.ipv4tunnels.waiting_removal if x['tunnel_ip'] not in subnet_ips]

    CURDB.ipv4tunnels.update()


def main_replace(options):
    affected_ips = masks_to_ip_list(options.subnet_masks.split(','))
    affected_instances = get_affected_instances(affected_ips)

    for group in CURDB.groups.get_groups():
        instances = group.get_instances()
        for instance in instances:
            if (instance.host.name, instance.port) in affected_instances:
                free_ip = CURDB.ipv4tunnels.free_with_delay(instance, group.card.properties.ipip6_ext_tunnel_pool_name)
                alloc_ip = CURDB.ipv4tunnels.alloc(instance, group.card.properties.ipip6_ext_tunnel_pool_name)

                assert free_ip == affected_instances[(instance.host.name, instance.port)]
                assert alloc_ip not in affected_ips

                print('Group {} ({}:{}) {} -> {}'.format(
                    group.card.name, instance.host.name, instance.port, free_ip, alloc_ip
                ))

    CURDB.ipv4tunnels.update()


def main_show_affected(options):
    affected_ips = masks_to_ip_list(options.subnet_masks.split(','))
    affected_instances = get_affected_instances(affected_ips)

    affected_groups = set()
    sun_instances_count = 0
    for group in CURDB.groups.get_groups():
        instances = group.get_instances()
        for instance in instances:
            if (instance.host.name, instance.port) in affected_instances:
                affected_groups.add(group.card.name)
                sun_instances_count += len(instances)
                break

    return {
        'affected_groups': list(affected_groups),
        'affected_insatnces_count': len(affected_instances),
        'affected_groups_instances_count': sun_instances_count,
    }


def main_show_subnets(options):
    subnets = {}
    for pool_name, pool in CURDB.ipv4tunnels.pool.items():
        subnets[pool_name] = []
        for subnet in pool.loc_pools:
            subnets[pool_name].append(str(subnet.subnet))
    return subnets


def main(options):
    if options.action == EActions.STATS:
        return main_stats(options)
    elif options.action == EActions.DIFF:
        return main_diff(options)
    elif options.action == EActions.EXPORT:
        return main_export(options)
    elif options.action == EActions.UPDATE:
        return main_update_pool_subnet(options)
    elif options.action == EActions.CLEANUP:
        return main_cleanup(options)
    elif options.action == EActions.FREE:
        return main_free(options)
    elif options.action == EActions.REPLACE:
        return main_replace(options)
    elif options.action == EActions.DUMP_REMOTE:
        return main_dump_remote(options)
    elif options.action == EActions.ADD_POOL:
        return main_add_pool(options)
    elif options.action == EActions.ADD_POOL_SUBNET:
        return main_add_pool_subnet(options)
    elif options.action == EActions.SHOW_AFFECTED:
        return main_show_affected(options)
    elif options.action == EActions.SHOW_SUBNETS:
        return main_show_subnets(options)
    else:
        raise Exception('Unknown action {}'.format(options.action))


def print_result(result, options):
    if options.action in (EActions.STATS, EActions.EXPORT, EActions.CLEANUP, EActions.FREE,
                          EActions.REPLACE, EActions.DUMP_REMOTE):
        print result
    elif options.action == EActions.DIFF:
        common_ips, different_fqdn_ips, extra_racktables_ips, extra_gencfg_ips = result
        print 'Common to gencfg/racktables ips ({} total): {}'.format(len(common_ips), ' '.join(sorted(common_ips)))
        print 'Common to gencfg/racktables ips with different fqdns ({} total): {}'.format(len(different_fqdn_ips), ' '.join(sorted(different_fqdn_ips)))
        print 'Extra racktables ips ({} total): {}'.format(len(extra_racktables_ips), ' '.join(sorted(extra_racktables_ips)))
        print 'Extra gencfg ips ({} total): {}'.format(len(extra_gencfg_ips), ' '.join(sorted(extra_gencfg_ips)))
    elif options.action == EActions.ADD_POOL:
        print 'Added pool <{}>'.format(options.pool_name)
    elif options.action == EActions.ADD_POOL_SUBNET:
        print 'Added subnet <{}> for pool <{}>'.format(options.subnet_descr, options.pool_name)
    elif options.action == EActions.UPDATE:
        print 'Updated'
    elif options.action == EActions.SHOW_AFFECTED:
        print(json.dumps(result, indent=4))
    elif options.action == EActions.SHOW_SUBNETS:
        print(json.dumps(result, indent=4))
    else:
        raise Exception('Unknown action {}'.format(options.action))


if __name__ == '__main__':
    options = get_parser().parse_cmd()

    normalize(options)

    result = main(options)

    print_result(result, options)
