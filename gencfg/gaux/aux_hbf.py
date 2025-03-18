"""Auxiliarily function for manipulating with mtn"""

import hashlib


def hbf_group_macro_name(group):
    """Return hbf macros for gencfg group"""
    return '_GENCFG_{}_'.format(group.card.name.strip('_'))


def generate_hbf_slb_info(group, slb_name, slb_ipv4_mtu, slb_ipv6_mtu, decapsulator_anycast_address):
    """GENCFG-1222

    :param slb_name: domain of slb, connected to group (e. g. test-cplb.yandex.net)
    :param slb_ipv4_mtu: MTU for ipv4 tunnels
    :param slb_ipv6_mtu: MTU for ipv6 tunnels
    :decapsulator_anycast_address: anycast decapsulator address
    """

    result = []
    for ip in group.parent.db.slbs.get(slb_name).ips:
        if ip.find(':') >= 0:
            ip_result = dict(type='ipv6', ip=ip)
            if slb_ipv6_mtu is not None:
                ip_result['mtu'] = slb_ipv6_mtu
            if decapsulator_anycast_address is not None:
                ip_result['decapsulator_anycast_address'] = decapsulator_anycast_address
            result.append(ip_result)
        else:
            ip_result = dict(type='ipv4', ip=ip)
            if slb_ipv4_mtu is not None:
                ip_result['mtu'] = slb_ipv4_mtu
            if decapsulator_anycast_address is not None:
                ip_result['decapsulator_anycast_address'] = decapsulator_anycast_address
            result.append(ip_result)

    return result


def _shorten(name, max_len):
    """Replace excess name tail with its hash"""
    assert max_len >= 4, name

    if len(name) <= max_len:
        return name

    return '{}_{}'.format(name[:max_len-4],
                          hashlib.md5(name).hexdigest()[:3].upper())


def _generate_mtn_hostname_old(instance, vlan_prefix):
    """Generate hostname for mtn/containered instance"""

    # ========================== SKYDEV-1121 FINISH ==================================
    max_prefix_len = 7
    assert len(vlan_prefix) <= max_prefix_len
    # ========================== SKYDEV-1121 START ===================================

    # generate result name
    prefix = '%s%s-' % (vlan_prefix, instance.host.name.partition('.')[0])
    postfix = '-%s.gencfg-c.yandex.net' % (instance.port)
    dnsname = '%s%s%s' % (
        prefix,
        _shorten(instance.type, 60 - len(postfix) - max_prefix_len),
        postfix,
    )

    dnsname = dnsname.replace('_', '-').lower()

    return dnsname


def generate_mtn_hostname(instance, group, vlan_prefix, ipv6addr=None):
    """Generate fully-qualified hostname for mtn instance"""

    mtn_fqdn_version = max(getattr(instance.host, 'mtn_fqdn_version', 0),
                           getattr(group.card.properties, 'mtn_fqdn_version', 0))

    # the very first version
    if mtn_fqdn_version == 0:
        name = _generate_mtn_hostname_old(instance, vlan_prefix)
        if len(name) <= 64:
            return name

    prefix = _get_prefix(instance, group, mtn_fqdn_version, vlan_prefix, ipv6addr)
    infix = _get_infix(instance, group, mtn_fqdn_version)
    postfix = _get_postfix(instance, group, mtn_fqdn_version)
    assert postfix.endswith('gencfg-c.yandex.net')

    max_name_len = _get_max_name_len(mtn_fqdn_version, vlan_prefix, instance)
    prefix = _shorten(prefix, max_name_len - len(postfix) - 4)
    infix = _shorten(infix, max_name_len - len(postfix) - len(prefix))

    if mtn_fqdn_version < 4:
        fqdn = '{}{}{}'.format(prefix, infix, postfix)
    else:
        if infix:
            infix = '-' + infix
        fqdn = '{}{}{}'.format(prefix, infix, postfix)

    return fqdn.replace('_', '-').lower()


def _get_prefix(instance, group, mtn_fqdn_version, vlan_prefix, ipv6addr):
    # {vlan}-{short_host}
    prefix = '{}{}'.format(vlan_prefix, instance.host.name.partition('.')[0])

    # RX-336: hostname must change whenever host ip is changed
    # {vlan}-{short_host}-{hash(ip)}
    if instance.host.mtn_fqdn_version == 1 or instance.host.mtn_fqdn_version >= 3 or mtn_fqdn_version >= 4:
        if ipv6addr is None:
            ipv6addr = generate_mtn_addr(instance, group, 'vlan688' if vlan_prefix == '' else 'vlan788')
        salt = getattr(group.card.properties.mtn, 'hash_salt', '') or ''
        if salt:
            assert group.card.name == 'SAS_YT_SENECA_RPC_PROXIES'  # protection; remove after first run
        prefix += '-{}'.format(hashlib.md5(ipv6addr + salt).hexdigest()[:3].upper())

    # preserve the old buggy behavior: '-' included in prefix
    if mtn_fqdn_version < 4:
        prefix += '-'

    return prefix


def _get_infix(instance, group, mtn_fqdn_version):
    infix = instance.type
    if mtn_fqdn_version >= 4:
        if getattr(group.card.properties, 'mtn_fqdn_domain', None):
            return ''
    return infix


def _get_postfix(instance, group, mtn_fqdn_version):
    default_postfix = '-{}.gencfg-c.yandex.net'.format(instance.port)
    if mtn_fqdn_version <= 3:
        return default_postfix

    domain = getattr(group.card.properties, 'mtn_fqdn_domain', None)
    return ('.' + domain) if domain else default_postfix


def _get_max_name_len(mtn_fqdn_version, vlan_prefix, instance):
    if mtn_fqdn_version in (0, 1):  # GENCFG-1900
        return 63 if vlan_prefix == '' else 64
    elif mtn_fqdn_version in (2, 3, 4):  # RX-478, RX-741
        return 60 if vlan_prefix == '' else 63  # hold place for fb-
    else:
        raise Exception('Mtn fqdn version <{}> for instance <{}> is not supported'.format(mtn_fqdn_version, instance.full_name()))


def generate_mtn_addr(instance, group, vlan_name):
    """Generate mtn address for instance"""
    net_as_addr = ':'.join(instance.host.vlans[vlan_name].split(':')[:4])
    ipv6addr = '%s:%x:%x:0:%x' % (net_as_addr, group.card.properties.hbf_project_id / (1 << 16),
                                  group.card.properties.hbf_project_id % (1 << 16), instance.port)
    return ipv6addr


def generate_hbf_info(group, instance):
    """Generate section with hbf info for instance (as requested in GENCFG-780)

    :param group: gencfg group to work with
    :type group: core.igroups.IGroup
    :param instance: instance of group specified as first parameter
    :type instance: cores.instances.TInstance

    Typical result is something like this:
        {
            "hbf_project_id": "10010fc",  # group hbf project id in hex
            "interfaces": {  # info on generated names and adresses
                 "backbone": {
                     "hostname": "sas1-5913-SAS_DIVERSITY_JUP_1E0-23678.gencfg-c.yandex.net",
                     "ipv6addr": "2a02:6b8:c08:190f:100:10fc:0:5c7e"
                 },
                 "fastbone": {
                     "hostname": "fb-sas1-5913-SAS_DIVERSITY_JUP_1E0-23678.gencfg-c.yandex.net",
                     "ipv6addr": "2a02:6b8:fc00:188f:100:10fc:0:5c7e"
                 }
             },
             "mtn_ready": true,
             "resolv.conf": "search search.yandex.net yandex.ru;nameserver 2a02:6b8:0:3400::1;nameserver 2a02:6b8::1:1;options timeout:1 attempts:1",
             # =============================== GENCFG-1222 START ========================================
             slb: [
                {
                    type: 'ipv4',
                    ip: ip1
                },
                {
                    type: 'ipv4',
                    ip: ip2,
                },
                ...
                {
                    type: 'ipv6',
                    ip: ip3,
                },
                ...
             ]
             # =============================== GENCFG-1222 FINISH =======================================
         }

    """

    # instance on virtual machine, do not generate hbf section
    if instance.host.is_vm_guest():
        return {}

    # group/instance not ready to generate addrs
    if not (hasattr(group.card.properties, 'hbf_project_id')) or (group.card.properties.hbf_project_id == 0):
        return {}

    if group.card.name not in generate_hbf_info.cache:
        result = {
            'hbf_project_id': '%x' % group.card.properties.hbf_project_id,
            'resolv.conf':  group.card.properties.hbf_resolv_conf or None,  # GENCFG-1700
            'mtn_ready': True,  # GENCFG-1738
        }

        # =================================== GENCFG-1222 START =================================================
        hbf_slb_ipv4_mtu = None
        hbf_slb_ipv6_mtu = None
        hbf_decapsulator_anycast_address = None
        hbf_slb_names = []
        if hasattr(group.card.properties, 'mtn') and hasattr(group.card.properties.mtn, 'tunnels') and \
                (group.card.properties.mtn.tunnels.hbf_slb_name is not None) and (group.card.properties.mtn.tunnels.hbf_slb_name != []):
                    hbf_slb_ipv4_mtu = group.card.properties.mtn.tunnels.hbf_slb_ipv4_mtu
                    hbf_slb_ipv6_mtu = group.card.properties.mtn.tunnels.hbf_slb_ipv6_mtu
                    hbf_decapsulator_anycast_address = group.card.properties.mtn.tunnels.hbf_decapsulator_anycast_address
                    hbf_slb_names = group.card.properties.mtn.tunnels.hbf_slb_name
        # =================================== GENCFG-1434 START =================================================
        if isinstance(hbf_slb_names, str):
            hbf_slb_names = [hbf_slb_names]
        if hbf_slb_names:
            result['slb'] = []
            for hbf_slb_name in hbf_slb_names:
                result['slb'].extend(generate_hbf_slb_info(group, hbf_slb_name, hbf_slb_ipv4_mtu, hbf_slb_ipv6_mtu, hbf_decapsulator_anycast_address))
        # =================================== GENCFG-1434 START ==================================================
        # =================================== GENCFG-1222 FINISH =================================================

        generate_hbf_info.cache[group.card.name] = result

    result = generate_hbf_info.cache[group.card.name].copy()
    result['interfaces'] = {}

    # generate vlan688
    if 'vlan688' in instance.host.vlans:
        vlan688_ipv6addr = generate_mtn_addr(instance, group, 'vlan688')
        result['interfaces']['backbone'] = {
            'ipv6addr': vlan688_ipv6addr,
            'hostname': generate_mtn_hostname(instance, group, '', vlan688_ipv6addr),
        }
    else:
        vlan688_ipv6addr = None

    # generate vlan788
    if 'vlan788' in instance.host.vlans:
        vlan788_ipv6addr = generate_mtn_addr(instance, group, 'vlan788')
        result['interfaces']['fastbone'] = {
            'ipv6addr': vlan788_ipv6addr,
            'hostname': generate_mtn_hostname(instance, group, 'fb-', vlan688_ipv6addr if vlan688_ipv6addr else vlan788_ipv6addr),
        }

    # =============================== GENCFG-1077 START ==============================================
    if group.card.properties.ipip6_ext_tunnel_v2:
        tunnel_addr = group.parent.db.ipv4tunnels.get(instance, getattr(group.card.properties, 'ipip6_ext_tunnel_pool_name', 'default'))
        if tunnel_addr is not None:
            result['tunnels'] = []
            result['tunnels'].append(dict(
                name='ipip6_ext_tun',
                ipv4_address=tunnel_addr,
                local_ipv6_address=generate_mtn_addr(instance, group, 'vlan688'),
                remote_ipv6_address='2a02:6b8:b010:a0ff::1',
            ))
    # =============================== GENCFG-1077 FINISH =============================================

    return result
generate_hbf_info.cache = dict()
