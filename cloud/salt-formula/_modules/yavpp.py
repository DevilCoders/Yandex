#!/usr/bin/python
# -*- coding: utf-8 -*-

"""helper module to prepare Yandex VPP configs"""

import collections
import copy
import ipaddress
import math
import re


class Core(object):
    """struct-like class to represent processor core"""
    def __init__(self, idx, numa, prio=0):
        self.idx = idx
        self.numa = numa
        self.prio = prio


class Range(object):
    """helper class to represent range of numbers"""
    def __init__(self, start):
        self.start = start
        self.stop = start

    def __str__(self):
        if self.stop == self.start:
            return str(self.start)
        elif self.stop == self.start + 1:
            return '{},{}'.format(self.start, self.stop)
        else:
            return '{}-{}'.format(self.start, self.stop)


def _collapse(worker_nos):
    """function to collapse list from [1,2,3,5,6,7] to '1-3,5-7'"""
    acc = []
    worker_nos.sort()
    itr = iter(worker_nos)
    elt = Range(itr.next())
    for val in itr:
        if val == elt.stop + 1:
            elt.stop = val
        else:
            acc.append(elt)
            elt = Range(val)
    acc.append(elt)
    return ','.join(str(x) for x in acc)


def _is_pow2(x):
    return x & (x - 1) == 0



def _cores_data(type, cores_list):
    return {
        '{}_cores'.format(type): _collapse(cores_list),
        '{}_cores_qty'.format(type): len(cores_list),
    }


def _get_cores(n_cores, numa, nics):
    """function to get cores workers should be started at"""
    reserves = {
        'kernel': 1,
        'gobgp': 0,
        'main': 1,
        'controller': 0
    }
    if n_cores > 2:
        reserves['main'] = 1
        reserves['gobgp'] = 1
        reserves['controller'] = 1
    if n_cores > 4:
        reserves['gobgp'] = 2
    if n_cores > 5:
        reserves['gobgp'] = 1
    if n_cores > 6:
        reserves['gobgp'] = 2
    num_workers = n_cores - sum(reserves.values())
    while not _is_pow2(num_workers):
        num_workers -= 1

    # TODO(@svartapetov) until CLOUD-18361 we have nothing to do with this cores
    free_cores = n_cores - num_workers - sum(reserves.values())
    reserves['workers'] = num_workers
    reserves['free'] = free_cores
    nic = nics[-1]
    num_queues = num_workers
    if num_queues > nic['queues']:
        raise ValueError("Can't create {} queues on {}".format(num_queues, nic))
    cores_per_numa = n_cores / numa
    cores = [Core(i, i / cores_per_numa, 1) for i in range(n_cores)]
    for core in [x for x in cores if x.numa == nic['numa_node']]:
        # See below. put workers on same numa node as nic
        core.prio = 2
    sorted_cores = sorted(cores, key=lambda x: x.prio)
    last_core = 0
    next_core = 0
    # We used to specify manually to ensure correct order
    types_to_process = ['kernel', 'controller', 'gobgp', 'free', 'main', 'workers']
    # sanity check
    assert set(reserves.keys()) == set(types_to_process)

    cpu_reserves = {}
    for reserve_type in types_to_process:
        last_core = next_core
        next_core = last_core + reserves[reserve_type]
        c_cores = [x.idx for x in sorted_cores[last_core: next_core]]
        if reserves[reserve_type] == 0:
            continue
        cpu_reserves[reserve_type] = c_cores

    # Support small vms
    if reserves['gobgp'] == 0:
        cpu_reserves['gobgp'] = cpu_reserves['kernel']
    # We used to set affinity for controllers even if there is not enough cores see CLOUD-17299
    # In this case use same cores as gobgp have
    if reserves['controller'] == 0:
        cpu_reserves['controller'] = cpu_reserves['gobgp']

    retval = {
        'num_rx_queues': num_queues,
        'num_tx_queues': num_queues
    }
    for type, reserve in cpu_reserves.items():
        retval.update(_cores_data(type, reserve))

    return retval


IP4_HEAPSIZE_MIN = 1024
IP4_HEAPSIZE_MAX = 4095
IP6_HEAPSIZE = 1024
HEAPSIZE = 1024
MBUF_SIZE = 2.5 / 1024.0
HUGEPAGE_EXTRA = 2
HUGEPAGE_SIZE = 2
MEM_USAGE_RATIO = 0.75


def _get_memory(mem, cores, numa, nics):
    """function to count memory needed"""
    nic = nics[-1]
    cores_per_numa = cores / numa
    num_mbufs = nic['queue_size'] * 2 * cores_per_numa * len(nics)
    mbufs_hugepages = int(math.ceil(num_mbufs * MBUF_SIZE / HUGEPAGE_SIZE)) + HUGEPAGE_EXTRA
    mbufs_size = mbufs_hugepages * numa * HUGEPAGE_SIZE
    available_mem = int((mem - mbufs_size) * MEM_USAGE_RATIO)
    required_mem = int(IP4_HEAPSIZE_MIN + IP6_HEAPSIZE + HEAPSIZE)
    if available_mem < required_mem:
        raise NotImplementedError('Not enough memory: {}Mb while {}Mb required'.format(available_mem, required_mem))
    ip4_heap = available_mem - IP6_HEAPSIZE - HEAPSIZE
    return {
        'ip4_heapsize':  '{}M'.format(min(IP4_HEAPSIZE_MAX, ip4_heap)),
        'ip6_heapsize':  '{}M'.format(IP6_HEAPSIZE),
        'heapsize':      '{}M'.format(HEAPSIZE),
        'num_mbufs':     num_mbufs,
        'socket_mem':    ','.join([str(mbufs_hugepages)] * numa),
        'num_hugepages': mbufs_hugepages * numa
    }


def vpp_conf(cloudgate_conf, grains):
    """helper function to produce vpp config"""
    cores = grains['num_cpus']
    numa = grains['num_sockets']
    mem = grains['mem_total']
    nics = grains['yavpp_ifaces']
    MIN_CORES = 2
    if cores < MIN_CORES:
        raise NotImplementedError("To run cgw we need at least {} cores.".format(MIN_CORES))

    if len(nics) < 4:
        raise RuntimeError("At least 4 interfaces is required for vpp to work.")
    retval = {
        'num_rx_desc': nics[-1]['queue_size'],
        'num_tx_desc': nics[-1]['queue_size']
    }
    retval.update(_get_cores(cores, numa, nics))
    retval.update(_get_memory(mem, cores, numa, nics))
    retval.update(_ifaces_conf(nics))
    return retval


def _ifaces_conf(nics):
    """helper function to produce interfaces part of config"""
    if len(nics) < 4:
        raise RuntimeError('4 VFs/VIFs per nic is implemented yet')
    skip = len(nics) / 4
    ifaces = collections.OrderedDict([
        ('vpp{}'.format(i), nic) for i, nic in enumerate(nics[skip:])
    ])
    return {
        'phyifaces': ifaces
    }


def _process_link_local(group_conf):
    """ Split address and interface for link-local peers """
    new_group = copy.deepcopy(group_conf)
    full_addresses = new_group['peer_addresses']
    addresses = [x.split('%')[0] for x in full_addresses]
    new_group['peer_addresses'] = addresses
    new_group['full_addresses'] = full_addresses
    return new_group


def _attached_routes(ifaces):
    """helper function to explicitly add attached routes"""
    routes = []
    for _, iface in ifaces.iteritems():
        addr = ipaddress.ip_interface(unicode(iface['addr']))
        routes.append({
            'route': str(addr.network),
            'dev': iface['os_name'],
            'src': str(addr.ip)
        })
    return routes


def _host_conf(cloudgate_conf):
    """helper function to generate host interfaces and routes config"""
    host = {
        'gateways': cloudgate_conf['host']['gateways'],
        'interfaces': cloudgate_conf['host']['interfaces'],
        'loopback': cloudgate_conf['host']['loopback'],
        'routes': _attached_routes(cloudgate_conf['host']['interfaces']) + cloudgate_conf['host']['routes']
    }
    if host['interfaces']['UPSTREAM_V4']:
        name = host['interfaces']['UPSTREAM_V4']['os_name']
        addr = [x for x in host['gateways'] if x['iface'] == name][0]['gateway']
        for route in host['routes']:
            if route.get('nexthop', None) == addr:
                route['src'] = host['loopback'].split('/')[0]
    return host


def go2vpp_conf(cloudgate_conf, grains):
    """helper function to produce go2vpp config"""
    nics = grains['yavpp_ifaces']
    vrf = cloudgate_conf['vrf']

    retval = _ifaces_conf(nics)
    retval.update({
        'router_id': cloudgate_conf['router_id'],
        'own_as': cloudgate_conf['own_as'],
        'log': cloudgate_conf['log'],
        'vrf': vrf,
        'downstream': cloudgate_conf['downstream'],
        'host': _host_conf(cloudgate_conf),
        'externals': [],
        'yaggr': cloudgate_conf['yaggr']
    })
    externals_v4 = []
    externals_v6 = []
    for announce in cloudgate_conf['downstream']['announces']:
        subnet = announce.split('via')[0].strip()
        if ':' in subnet:
            externals_v6.append(subnet)
        else:
            externals_v4.append(subnet)
    if 'upstream6' in cloudgate_conf:
        retval['upstream6'] = _process_link_local(cloudgate_conf['upstream6'])
    if 'upstream4' in cloudgate_conf:
        retval['upstream4'] = cloudgate_conf['upstream4']
    if 'reflector' in cloudgate_conf:
        retval['reflector'] = cloudgate_conf['reflector']
        retval['externals'] = externals_v4
    if 'cloud_border' in cloudgate_conf:
        retval['cloud_border'] = cloudgate_conf['cloud_border']
    addrbytes = cloudgate_conf['router_id'].split('.')
    retval['veth'] = {
        'address-v6': 'fc00::6a7e:' + ':'.join(addrbytes),
        'address-v4': '100.64.{}.{}'.format(addrbytes[2], addrbytes[3]),
        'routes-v4': externals_v4,
        'routes-v6': externals_v6
    }
    return retval


def bgp2vpp_conf(cloudgate_conf, grains):
    """helper function to produce bgp2vpp config"""
    # TODO(bayrinat): change it, when sets change to bootstrap
    nics = grains['yavpp_ifaces']
    host = cloudgate_conf['host']
    if host['interfaces']['UPSTREAM_V4']:
        name = host['interfaces']['UPSTREAM_V4']['os_name']
        addr = [x for x in host['gateways'] if x['iface'] == name][0]['gateway']
        for route in host['routes']:
            if route['nexthop'] == addr:
                route['src'] = host['loopback'].split('/')[0]
    retval = _ifaces_conf(nics)
    retval["log"] = cloudgate_conf['log']
    retval["host"] = host
    # prepare announces
    upstream_v4_announces = []
    upstream_v6_announces = []
    reflector_announces = []
    downstream_v4_announces = []
    downstream_v6_announces = []
    for announce in cloudgate_conf['upstream4']['announces']:
        upstream_v4_announces.append(bgp2vpp_announce_prepare(announce))
    for announce in cloudgate_conf['upstream6']['announces']:
        upstream_v6_announces.append(bgp2vpp_announce_prepare(announce))
    for announce in cloudgate_conf['reflector']['announces']:
        reflector_announces.append(bgp2vpp_announce_prepare(announce))
    for announce in cloudgate_conf['downstream']['announces']:
        subnet = announce.split('via')[0].strip()
        if ':' in subnet:
            downstream_v6_announces.append(bgp2vpp_announce_prepare(announce))
        else:
            downstream_v4_announces.append(bgp2vpp_announce_prepare(announce))
    retval['upstream_v4_announces'] = upstream_v4_announces
    retval['upstream_v6_announces'] = upstream_v6_announces
    retval['reflector_announces'] = reflector_announces
    retval['downstream_v4_announces'] = downstream_v4_announces
    retval['downstream_v6_announces'] = downstream_v6_announces

    retval['upstream4'] = cloudgate_conf['upstream4']
    retval['upstream6'] = _process_link_local(cloudgate_conf['upstream6'])
    retval['downstream'] = cloudgate_conf['downstream']
    retval['reflector'] = cloudgate_conf['reflector']

    return retval


def bgp2vpp_announce_prepare(announce):
    """helper function to format announce string"""
    parts = announce.split(" ")
    if len(parts) < 3:
        raise RuntimeError("Unknown announce format")

    if "nat" in parts[2]:
        raise NotImplementedError("NAT is not implemented yet")

    if "/" not in parts[0]:
        prefix_len = "128" if ":" in parts[0] else "32"
        parts[0] = "{}/{}".format(parts[0], prefix_len)

    if len(parts) == 3:
        fstr = "{} nexthop {}".format(parts[0], parts[2])
    else:
        if "label" in parts[3]:
            fstr = "{} label {} nexthop {}".format(parts[0], parts[4], parts[2])
            parts = parts[5:]
        else:
            fstr = "{} nexthop {}".format(parts[0], parts[2])
            if len(parts) > 3:
                parts = parts[3:]

        for part in parts:
            if "comm" in part:
                part = "community"
            fstr = fstr.__add__(" " + part)

    fstr = fstr.__add__(" origin igp")
    return fstr


def dler_conf(dler_config, grains):
    """helper function to produce dler config"""
    nics = grains['yavpp_ifaces']
    retval = _ifaces_conf(nics)
    retval.update({
        'upstream': dler_config['upstream'],
        'downstream': dler_config['downstream'],
        'host': dler_config['host'],
        'routes': _routes_conf(dler_config['host']['interfaces']),
        'anycastaddrs': _addrs_conf(dler_config['downstream']),
        'ip6ifaces': _ip6ifaces_conf(dler_config['host']['interfaces'])
    })
    return retval


def _routes_conf(host_config):
    """helper function to produce routes part of config"""
    routes = collections.defaultdict(list)
    for host_cfg in host_config:
        for _, cfglist in host_cfg.iteritems():
            for cfg in cfglist:
                route = cfg.get('route')
                gateway = cfg.get('gateway')
                if route is None or gateway is None:
                    continue
                routes[route].append(gateway)
    return routes


def _addrs_conf(down_conf):
    """helper function to collect anycast addresses"""
    anns = {x for _, cfg in down_conf.iteritems() for x in cfg['announce']}
    return [x.replace('/128', '/32') for x in anns]


def _ip6ifaces_conf(host_config):
    """helper function to assign fake ip4 addrs for pure ip6 interfaces"""
    ifaces = []
    for host_cfg in host_config:
        for iface, cfglist in host_cfg.iteritems():
            addrs = [cfg.get('address', ':') for cfg in cfglist]
            is_ip6 = [':' in addr for addr in addrs]
            if any(is_ip6):
                ifaces.append(iface)
    return {name: '127.0.0.{}/32'.format(127 + i)
            for i, name in enumerate(ifaces)}


def _net_prefix(pfx_str):
    """helper function to get common string prefix for net hosts"""
    pfx = ipaddress.ip_network(unicode(pfx_str))
    first = str(pfx.network_address)
    last = str(pfx.broadcast_address)
    retval = []
    for i in xrange(min(len(first), len(last))):
        if first[i] == last[i]:
            retval.append(first[i])
        else:
            break
    return ''.join(retval)


def pfx_re(pfx_list):
    """helper function to generate regexp for given net hosts"""
    if not pfx_list:
        return '^nomatch'
    if all(':' not in x for x in pfx_list):
        tmpl = '^({})[0-9\.]*(?=/32)'
    elif all(':' in x for x in pfx_list):
        tmpl = '^({})[0-9a-f:]*(?=/128)'
    else:
        raise ValueError('mixed set of both ip4/ip6 prefixes {}'.format(pfx_list))
    pfx_strs = [_net_prefix(x) for x in pfx_list]
    return tmpl.format('|'.join(re.escape(x) for x in pfx_strs))


def pfxs_v4(announces):
    """helper function to return ipv4 prefixes"""
    pfxs = [x.split('via')[0].strip() for x in announces]
    return [x for x in pfxs if ':' not in x]


def pfxs_v6(announces):
    """helper function to return ipv6 prefixes"""
    pfxs = [x.split('via')[0].strip() for x in announces]
    return [x for x in pfxs if ':' in x]


def prefix_overlaps(left, right):
    """ check if prefixes are overlapping """
    return ipaddress.ip_network(unicode(left)).overlaps(ipaddress.ip_network(unicode(right)))


#-----------------------------------
def cgw_conf(cloudgate_conf, host_tags, grains):
    """cgw configs helper function, v2"""
    hostname = grains['nodename']
    base_role = grains['cluster_map']['hosts'][hostname]['base_role']
    reflector_enabled = not cloudgate_conf.get('reflector', {'disabled': True})['disabled']
    downstream_announces = cloudgate_conf.get('downstream', {'announces': []})['announces']
    fip_prefixes = [
        x.split('via')[0].strip()
        for x in downstream_announces if 'nat' not in x
    ]
    ip4_prefixes = [x for x in fip_prefixes if ':' not in x]

    is_ip4 = reflector_enabled and (base_role == 'honey-cgw' or bool(ip4_prefixes))
    is_ip6 = not cloudgate_conf.get('upstream6', {'disabled': True})['disabled']
    is_nat44 = any('nat' in x for x in downstream_announces)
    is_dc4 = bool(cloudgate_conf.get('direct_connect'))
    is_rkn = 'rkn-cgw' in host_tags
    #TODO: what load-balancer omen is?
    is_lb4 = False
    is_lb6 = False
    retval = {
        'capabilities': {
            'ip4': is_ip4,
            'ip6': is_ip6,
            'nat44': is_nat44,
            'dc4': is_dc4,
            'lb4': is_lb4,
            'lb6': is_lb6,
            'rkn': is_rkn
        },
        'router_id': cloudgate_conf['router_id'],
        'log': cloudgate_conf['log'],
        'phyifaces': _new_ifaces_conf(grains['yavpp_ifaces'])
    }
    retval['host'] = _new_host_conf(cloudgate_conf, retval['phyifaces'])
    retval['peers'] = _generate_peers(cloudgate_conf, retval['capabilities'])
    retval['communities'] = _generate_comms(cloudgate_conf, retval['capabilities'])
    retval['nat44'] = _generate_nat(cloudgate_conf, retval['capabilities'])
    retval['rtargets'] = _generate_rts(cloudgate_conf, retval['capabilities'], retval['nat44'])
    # due to contrail mixing ip4 and ip6 downstream politics are special
    _generate_downstream_rts(cloudgate_conf, retval['capabilities'], retval)
    retval['announces'] = _generate_announces(cloudgate_conf, retval['capabilities'], retval['nat44'])
    retval['vrfs'] = _generate_vrfs(retval, cloudgate_conf)
    return retval


def _ip4_capable(capas):
    return any(capas[x] for x in ('ip4', 'nat44', 'dc4', 'lb4', 'rkn'))


def _generate_peers(cloudgate_conf, capas):
    """peer list configuration"""
    down_families = []
    if _ip4_capable(capas):
        down_families.append('l3vpn-ipv4-unicast')
    if capas['ip6'] or capas['lb6']:
        down_families.append('l3vpn-ipv6-unicast')
    retval = {
        'downstream': {
            'peers': cloudgate_conf['downstream']['peer_addresses'],
            'peer_addrs': cloudgate_conf['downstream']['peer_addresses'],
            'afisafi': down_families,
            'peer_as': cloudgate_conf['downstream']['peer_as'],
            'local_as': cloudgate_conf['downstream']['local_as'],
            'med': cloudgate_conf['downstream']['med']
        }
    }
    retval['reflector'] = {
        'peers': cloudgate_conf['reflector']['peer_addresses'],
        'peer_addrs': cloudgate_conf['reflector']['peer_addresses'],
        'afisafi': [],
        'peer_as': cloudgate_conf['reflector']['peer_as'],
        'local_as': cloudgate_conf['reflector']['local_as'],
        'med': cloudgate_conf['reflector']['med']
    }
    border_enabled = not cloudgate_conf.get('cloud_border', {'disabled': True})['disabled']
    if border_enabled:
        retval['cloud_border'] = {
             'peers': cloudgate_conf['cloud_border']['peer_addresses'],
             'peer_addrs': cloudgate_conf['cloud_border']['peer_addresses'],
             'afisafi': [],
             'peer_as': cloudgate_conf['cloud_border']['peer_as'],
             'local_as': cloudgate_conf['cloud_border']['local_as'],
             'med': cloudgate_conf['cloud_border']['med']
        }
    if capas['ip4'] or capas['nat44']:
        retval['reflector']['afisafi'].append('ipv4-unicast')
        if border_enabled:
            retval['cloud_border']['afisafi'].append('ipv4-unicast')
    if capas['dc4']:
        retval['reflector']['afisafi'].append('l3vpn-ipv4-unicast')
    if capas['ip6']:
        retval['reflector']['afisafi'].append('l3vpn-ipv6-unicast')
        if border_enabled:
            retval['cloud_border']['afisafi'].append('l3vpn-ipv6-unicast')
    retval['upstream4'] = {
        'peers': cloudgate_conf['upstream4']['peer_addresses'],
        'peer_addrs': cloudgate_conf['upstream4']['peer_addresses'],
        'afisafi': ['ipv4-labelled-unicast'],
        'peer_as': cloudgate_conf['upstream4']['peer_as'],
        'local_as': cloudgate_conf['upstream4']['local_as'],
        'med': cloudgate_conf['upstream4']['med']
    }
    return retval


def _generate_comms(cloudgate_conf, capas):
    """communities configuration"""
    retval = {'import': collections.defaultdict(list), 'export': collections.defaultdict(list)}
    retval['export']['upstream4'].append(__pillar__['comms']['upstream']['export']['cgw'])
    retval['export']['upstream4'].append(__pillar__['comms']['upstream']['export']['ycnets'])
    if _ip4_capable(capas):
        retval['import']['upstream4'].append(__pillar__['comms']['upstream']['import']['border'])
    if capas['dc4']:
        retval['import']['upstream4'].append(__pillar__['comms']['upstream']['import']['dlbr'])
        # CLOUD-24475: allow trafic between cgw-dc for e2e
        cgw_dc_comm_lo = __pillar__['comms']['upstream']['export']['cgw-dc-lo']
        retval['import']['upstream4'].append(cgw_dc_comm_lo)
        retval['export']['upstream4'].append(cgw_dc_comm_lo)
        retval['export']['reflector'].append(__pillar__['comms']['reflector']['export']['cgw-dc-agg'])
    if capas['ip4'] or capas['nat44']:
        retval['import']['reflector'].append(__pillar__['comms']['reflector']['import']['ip4'])
        retval['export']['reflector'].append(__pillar__['comms']['reflector']['export']['ip4'])
    if capas['rkn']:
        retval['import']['upstream4'].append(__pillar__['comms']['upstream']['import']['rkn'])
        retval['import']['reflector'].append(__pillar__['comms']['reflector']['import']['rknall'])
        retval['import']['reflector'].append(__pillar__['comms']['reflector']['import']['rknblack'])
    if capas['ip6']:
        retval['import']['upstream4'].append(__pillar__['comms']['upstream']['import']['ca'])
    return retval


def _any_to_list(x):
    if type(x) == list:
        return x
    else:
        return [x]


def _generate_rts(cloudgate_conf, capas, natconf):
    """route targets configuration"""
    retval = {'import': collections.defaultdict(list), 'export': collections.defaultdict(list)}
    if capas['ip4']:
        retval['export']['downstream'] += ['rt:{}'.format(x) for x in _any_to_list(cloudgate_conf['vrf']['export_route_targets']['ip4'])]
    if capas['ip6']:
        retval['export']['downstream'] += ['rt:{}'.format(x) for x in _any_to_list(cloudgate_conf['vrf']['export_route_targets']['ip6'])]
        retval['import']['reflector'].extend(_get_hbf_rts(cloudgate_conf))
        retval['export']['reflector'].extend(_get_hbf_rts(cloudgate_conf))
    if capas['nat44']:
        if natconf['shared']:
            retval['export']['downstream'].extend(
                'rt:{}'.format(x) for x in cloudgate_conf['vrf']['tenant_route_targets']
            )
        for conf in natconf['private']:
            retval['export']['downstream'].append('rt:{}'.format(conf['rt']))
    if capas['dc4']:
        for dc in cloudgate_conf['direct_connect']:
            retval['export']['downstream'].append('rt:{}'.format(dc['downstream_export']))
            retval['import']['reflector'].append('rt:{}'.format(dc['upstream_import']))
            retval['export']['reflector'].append('rt:{}'.format(dc['upstream_export']))
    return retval


def _generate_downstream_rts(cloudgate_conf, capas, retval):
    retval['downstream_ip4_import'] = []
    retval['downstream_ip6_import'] = []
    retval['downstream_ip4_export'] = []
    retval['downstream_ip6_export'] = []
    retval['fip_ip4_imports'] = ['rt:{}'.format(x) for x in _any_to_list(cloudgate_conf['vrf']['import_route_targets']['ip4'])]
    retval['fip_ip4_exports'] = ['rt:{}'.format(x) for x in _any_to_list(cloudgate_conf['vrf']['export_route_targets']['ip4'])]
    retval['fip_ip6_imports'] = ['rt:{}'.format(x) for x in _any_to_list(cloudgate_conf['vrf']['import_route_targets']['ip6'])]
    retval['fip_ip6_exports'] = ['rt:{}'.format(x) for x in _any_to_list(cloudgate_conf['vrf']['export_route_targets']['ip6'])]
    if capas['ip4']:
        retval['downstream_ip4_import'] += retval['fip_ip4_imports']
        retval['downstream_ip4_export'] += retval['fip_ip4_exports']
    if capas['ip6']:
        retval['downstream_ip6_import'] += retval['fip_ip6_imports']
        retval['downstream_ip6_export'] += retval['fip_ip6_exports']
    if capas['dc4']:
        for dc in cloudgate_conf['direct_connect']:
            retval['downstream_ip4_import'].append('rt:{}'.format(dc['downstream_import']))
    if capas['nat44']:
        if retval['nat44']['shared']:
            all_subnets_rts = [ 'rt:{}'.format(x) for x in cloudgate_conf['vrf']['tenant_route_targets']]
            retval['downstream_ip4_import'].extend(all_subnets_rts)
            retval['downstream_ip4_export'].extend(all_subnets_rts)
        for conf in retval['nat44']['private']:
            subnet_rt = 'rt:{}'.format(conf['rt'])
            retval['downstream_ip4_import'].append(subnet_rt);
            retval['downstream_ip4_export'].append(subnet_rt);


def _mangle_announces(announces, vrf):
    """mangle announces while bootstrap's untouched"""
    retval = []
    for announce in announces:
        if isinstance(announce, str):
            route = announce.split('via')[0].strip()
            retval.append({
                'route': route,
                'vrf': vrf
            })
        else:
            retval.append(announce)
    return retval


def _get_nat_fip4_fip6_announces(cloudgate_conf):
    downstream_announces = cloudgate_conf.get('downstream', {'announces': []})['announces']
    nat, fip4, fip6 = [], [], []
    for announce in downstream_announces:
        if 'nat' in announce:
            nat.append(announce)
        elif ':' not in announce.split('via')[0]:
            fip4.append(announce)
        else:
            fip6.append(announce)

    return nat, fip4, fip6


def _get_hbf_rt_by_prefix(prefix):
    ip_net = ipaddress.ip_network(unicode(prefix))
    cut_net_len = __pillar__['hbf_rt_by_prefix']['cut_prefix_len']
    key_prefix = str(ip_net.supernet(ip_net.prefixlen - cut_net_len))
    return __pillar__['hbf_rt_by_prefix']['prefixes'].get(key_prefix)


def _get_hbf_rts(cloudgate_conf):
    _, _, fip6 = _get_nat_fip4_fip6_announces(cloudgate_conf)
    result = []
    for x in fip6:
        prefix = x.split('via')[0].strip()
        rt = _get_hbf_rt_by_prefix(prefix)
        if rt and rt not in result:
            result.append(rt)
    if len(result) > 1:
        raise AttributeError("Too many rts generated for 1 cgw: {}.".format(result))
    return result


def _generate_announces(cloudgate_conf, capas, natconf):
    """announces configuration"""
    retval = collections.defaultdict(list)
    _, fip4_announces, fip6_announces = _get_nat_fip4_fip6_announces(cloudgate_conf)
    retval['upstream4'].extend(_mangle_announces(cloudgate_conf['upstream4']['announces'], 'loopbacks'))
    if capas['ip4']:
        retval['downstream'].extend(_mangle_announces(fip4_announces, 'ip4-contrail'))
        retval['reflector'].extend(_mangle_announces(cloudgate_conf['reflector']['announces'], 'ip4-reflector'))
    if capas['ip6']:
        retval['downstream'].extend(_mangle_announces(fip6_announces, 'ip6-contrail'))
        retval['reflector'].extend(_mangle_announces(cloudgate_conf['upstream6']['announces'], 'ip6-reflector'))
    if capas['dc4']:
        announces = [
            {'route': prefix, 'vrf': 'dc4-down-{}'.format(idx)}
            for idx, dc in enumerate(cloudgate_conf['direct_connect'])
            for prefix in dc['prefixes']
        ]
        retval['downstream'].extend(announces)
        retval['reflector'].extend([])
    if capas['nat44']:
        retval['downstream'].extend([
            {'route': x, 'vrf': 'nat-prefixes'} for x in natconf['shared']
        ])
        retval['downstream'].extend([
            {'route': x['prefix'], 'vrf': 'nat-prefixes'} for x in natconf['private']
        ])
        if 'reflector' not in retval:
            retval['reflector'] = []
    return retval


def _generate_vrfs(conf, cloudgate_conf):
    """vrfs configuration"""
    capas = conf['capabilities']
    dcconf = cloudgate_conf.get('direct_connect', [])
    retval = [{
        'name': 'loopbacks',
        'vrf': 0,
        'import': conf['communities']['import']['upstream4'],
        'export': conf['communities']['export']['upstream4'],
        'label': 3,
        'is_white': False,
        'is_nated': False,
        'is_ip6': False,
        'fuse': 'none'
    }]
    if capas['ip4'] or capas['nat44']:
        retval.append({
            'name': 'ip4-reflector',
            'vrf': 1,
            'import': conf['communities']['import']['reflector'],
            'export': conf['fip_ip4_exports'],
            'label': 44,
            'is_white': False,
            'is_nated': False,
            'is_ip6': False,
            'fuse': 'drop'
        })
    if capas['ip4']:
        retval.append({
            'name': 'ip4-contrail',
            'vrf': 1,
            'import': conf['fip_ip4_imports'],
            'export': conf['communities']['export']['reflector'],
            'label': None,
            'is_white': False,
            'is_nated': False,
            'is_ip6': False,
            'fuse': 'drop'
        })
    if capas['ip6']:
        retval.extend([
            {
                'name': 'ip6-reflector',
                'vrf': 0,
                'import': _get_hbf_rts(cloudgate_conf),
                'export': conf['fip_ip6_exports'],
                'label': 66,
                'is_white': False,
                'is_nated': False,
                'is_ip6': True,
                'fuse': 'drop'
            },
            {
                'name': 'ip6-contrail',
                'vrf': 0,
                'import': conf['fip_ip6_imports'],
                'export': [
                    __pillar__['comms']['reflector']['export']['noc6'],
                    __pillar__['comms']['reflector']['export']['internal6']
                ] + _get_hbf_rts(cloudgate_conf),
                'label': 2,
                'is_white': False,
                'is_nated': False,
                'is_ip6': True,
                'fuse': 'drop'
            }
        ])
    if capas['dc4']:
        label_start = 201
        for idx, dc in enumerate(dcconf):
            retval.extend([
                {
                    'name': 'dc4-up-{}'.format(idx),
                    'vrf': label_start + idx,
                    'import': ['rt:{}'.format(dc['upstream_import'])],
                    'export': ['rt:{}'.format(dc['downstream_export'])],
                    'label': label_start + idx,
                    'is_white': False,
                    'is_nated': False,
                    'is_ip6': False,
                    'fuse': 'drop'
                },
                {
                    'name': 'dc4-down-{}'.format(idx),
                    'vrf': label_start + idx,
                    'import': ['rt:{}'.format(dc['downstream_import'])],
                    'export': ['rt:{}'.format(dc['upstream_export'])],
                    'label': label_start + idx,
                    'is_white': False,
                    'is_nated': False,
                    'is_ip6': False,
                    'fuse': 'drop'
                }
            ])
    if capas['nat44']:
       retval.append ({
           'name': 'nat-prefixes',
           'vrf': 1,
           'import': [],
           'export': conf['communities']['export']['reflector'],
           'label': None,
           'is_white': False,
           'is_nated': False,
           'is_ip6': False,
           'fuse': 'drop'
       })
       # если есть коммуналка, отдельно создавать приватные NAT бессмысленно:
       # есть фильтр %asn%:* и автосоздание vrf'ов. Приваты подпадают под фильтр и vrf будут созданы
       if not conf['nat44']['shared']:
           for natconf in conf['nat44']['private']:
               rt = 'rt:{}'.format(natconf['rt'])
               retval.append({
                   'name': 'nat44-{}'.format(natconf['vrf']),
                   'vrf': natconf['vrf'],
                   'import': [rt],
                   'export': [rt],
                   'label': natconf['vrf'],
                   'is_white': True,
                   'is_nated': True,
                   'is_ip6': False,
                   'fuse': 'drop'
               })
    return retval


def _generate_nat(cloudgate_conf, capas):
    if not capas['nat44']:
        return {
           'shared': [],
           'tenant_asn': [],
           'private': []
        }
    downstream_announces = cloudgate_conf.get('downstream', {'announces': []})['announces']
    nat_announces = [x for x in downstream_announces if 'nat' in x]
    shared_announces = [x for x in nat_announces if 'via nat shared' in x]
    named_announces = [x for x in nat_announces if 'via nat shared' not in x]
    tenant_asn = set(
        x.split(':')[0] for x in cloudgate_conf['vrf']['tenant_route_targets']
    )
    if len(named_announces) > 100:
        raise RuntimeError("We've reserved 100 mpls labels for NAT. It's not enought any longer")
    # nat mpls labels are from 101 to 200. It is direct connect starting from 201
    label_start = 101
    confs = []
    for idx, ann in enumerate(named_announces):
        prefix, rt = [x.strip() for x in ann.split('via nat')]
        confs.append({'prefix': prefix, 'rt': rt, 'vrf': label_start + idx})
    shared = [x.split(' via')[0] for x in shared_announces]
    return {
        'shared': shared,
        'tenant_asn': list(tenant_asn),
        'private': confs
    }


def new_vpp_conf(cloudgate_conf, grains):
    """helper function to produce vpp config"""
    cores = grains['num_cpus']
    numa = grains['num_sockets']
    mem = grains['mem_total']
    MIN_CORES = 2
    if cores < MIN_CORES:
        raise NotImplementedError("To run cgw we need at least {} cores.".format(MIN_CORES))
    retval = {'phyifaces': _new_ifaces_conf(grains['yavpp_ifaces'])}
    nics = list(retval['phyifaces'].itervalues())
    retval.update({
        'num_rx_desc': nics[-1]['queue_size'],
        'num_tx_desc': nics[-1]['queue_size']
    })
    retval.update(_get_cores(cores, numa, nics))
    retval.update(_get_memory(mem, cores, numa, nics))
    return retval


def _new_ifaces_conf(nics):
    """helper function to produce interfaces part of config"""
    if len(nics) != 4:
        raise RuntimeError('4 VFs/VIFs per nic is implemented yet')
    # nic #0 is management, nic #2 is unused ip6 hbf vlan
    nics[1]['os_name'] = 'vpp0'
    nics[3]['os_name'] = 'vpp1'
    return {
        'DOWNSTREAM': nics[1],
        'UPSTREAM_V4': nics[3]
    }


def _new_host_conf(cloudgate_conf, phyifaces):
    """helper function to generate host interfaces and routes config"""
    ifaces = {}
    for name, iface in phyifaces.iteritems():
        ifaces[name] = {
            'os_name': iface['os_name'],
            'addr': cloudgate_conf['host']['interfaces'][name]['addr']
        }
    gateways = []
    for gwdesc in cloudgate_conf['host']['gateways']:
        confiface = [
            x for x, desc in cloudgate_conf['host']['interfaces'].iteritems()
            if desc['os_name'] == gwdesc['iface']
        ][0]
        gateways.append({'gateway': gwdesc['gateway'], 'iface': ifaces[confiface]['os_name']})
    host = {
        'gateways': gateways,
        'interfaces': ifaces,
        'loopback': cloudgate_conf['host']['loopback'],
        'routes': _attached_routes(ifaces) + cloudgate_conf['host']['routes']
    }
    name = cloudgate_conf['host']['interfaces']['UPSTREAM_V4']['os_name']
    addr = [x for x in cloudgate_conf['host']['gateways'] if x['iface'] == name][0]['gateway']
    for route in host['routes']:
        if route.get('nexthop', None) == addr:
            route['src'] = host['loopback'].split('/')[0]
    return host
