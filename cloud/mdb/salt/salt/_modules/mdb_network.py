# -*- coding: utf-8 -*-
"""
MDB module with some useful network related functions.
"""

import ipaddress

__salt__ = {}


def __virtual__():
    return True


CLOUD_YQL_PROD_PROJECT_ID = '0xf80a'  # _CLOUDYQLPRODNETS_
CLOUD_BEAVER_PREPROD_PROJECT_ID = '0xfc72'  # _CLOUD_MDB_CLOUDBEAVER_PREPROD_NETS_
CLOUD_BEAVER_PROD_PROJECT_ID = '0xf898'  # _CLOUD_MDB_CLOUDBEAVER_PROD_NETS_
YANDEX_QUERY_PREPROD_PROJECT_ID = '0xfc58'  # _CLOUD_YANDEXQUERY_PREPROD_NETS_
YANDEX_QUERY_PROD_PROJECT_ID = '0xf889'  # _CLOUD_YANDEXQUERY_PROD_NETS_
DATA_TRANSFER_PREPROD_PROJECT_ID = '0xfc11'  # _CLOUD_TM_PREPROD_NETS_
DATA_TRANSFER_PROD_PROJECT_ID = '0xf826'  # _CLOUD_TM_PROD_NETS_


def ip6_for_interface(iface):
    """
    Returns main IPv6 address for interface and None in case of absense.
    """
    grains_key = 'ip6_interfaces:{iface}'.format(iface=iface)
    addrs = _grains(grains_key)
    if not addrs:
        return None
    return _get_routable_ip6_addr(addrs)


def ip6_interfaces():
    """
    Returns dict where key is network interface name and value is IPv6 address.
    I.e. {
        "eth0": None,
        "eth1": "2a02:6b8:c0e:501:0:f804:0:20e"
    }
    """
    result = {}

    interfaces = _grains('ip6_interfaces')
    for iface, addrs in interfaces.items():
        if iface == 'lo':
            continue
        routable_addr = _get_routable_ip6_addr(addrs)
        if routable_addr is not None:
            result[iface] = routable_addr

    return result


def _get_routable_ip6_addr(addresses):
    """
    Returns main IPv6 address for interface and None in case of absense.
    """
    result = None
    for addr in addresses:
        if addr.startswith('fe80::') or addr.startswith('::') or addr.startswith('fd01:'):
            continue
        result = addr

    return result


def resolve_fqdn(fqdn, addr_type='A', nameserver=None):
    """
    Wrapper for dnsutil.[A|AAAA]
    """
    func = None
    if addr_type == 'A':
        func = 'dnsutil.A'
    elif addr_type == 'AAAA':
        func = 'dnsutil.AAAA'
    else:
        raise ValueError("Wrong addr_type {}".format(addr_type))

    ret = __salt__[func](fqdn, nameserver=nameserver)
    if type(ret) != list:
        raise Exception("Unable to get {} record for {}: {}".format(addr_type, fqdn, ret))

    return ret


def get_project_id(address):
    """
    Returns hex representation of project_id from IPv6 address
    """
    ip = ipaddress.ip_address(address)
    first96bits = int(ip) // 2**32
    project_id = first96bits % 2**32
    return hex(int(project_id))


def get_external_project_ids(exclude_user_project_id=False):
    """
    Return external project ids.
    """
    project_ids_with_action = get_external_project_ids_with_action(exclude_user_project_id)
    return [project_id for project_id, action in project_ids_with_action.items() if action == 'ACCEPT']


def get_external_project_ids_with_action(exclude_user_project_id=False):
    """
    Return firewall actions for external project ids.
    """
    user_ip6_addr = ip6_interfaces().get('eth0')
    user_project_id = get_project_id(user_ip6_addr) if user_ip6_addr else None

    is_testing = __salt__['dbaas.is_testing']()

    data_lens_enabled = _pillar('data:access:data_lens', False)
    metrika_enabled = _pillar('data:access:metrika', False)
    serverless_enabled = _pillar('data:access:serverless', False)
    web_sql_enabled = _pillar('data:access:web_sql', False)
    yandex_query_enabled = _pillar('data:access:yandex_query', False)
    data_transfer_enabled = _pillar('data:access:data_transfer', False)

    is_porto = False
    if _pillar('data:dbaas:vtype') == "porto":
        is_porto = True

    reject_enabled = _pillar('firewall:reject_enabled', False)

    external_project_ids_with_action = {}

    for external_project_id in _pillar('data:external_project_ids', []):
        external_project_ids_with_action[external_project_id] = 'ACCEPT'

    if web_sql_enabled:
        external_project_ids_with_action[CLOUD_YQL_PROD_PROJECT_ID] = 'ACCEPT'
        external_project_ids_with_action[CLOUD_BEAVER_PROD_PROJECT_ID] = 'ACCEPT'
    elif is_porto and reject_enabled:
        external_project_ids_with_action[CLOUD_YQL_PROD_PROJECT_ID] = 'REJECT'
        external_project_ids_with_action[CLOUD_BEAVER_PROD_PROJECT_ID] = 'REJECT'
    if web_sql_enabled and is_testing:
        external_project_ids_with_action[CLOUD_BEAVER_PREPROD_PROJECT_ID] = 'ACCEPT'
    elif is_porto and reject_enabled:
        external_project_ids_with_action[CLOUD_BEAVER_PREPROD_PROJECT_ID] = 'REJECT'

    if yandex_query_enabled:
        external_project_ids_with_action[YANDEX_QUERY_PROD_PROJECT_ID] = 'ACCEPT'
    elif is_porto and reject_enabled:
        external_project_ids_with_action[YANDEX_QUERY_PROD_PROJECT_ID] = 'REJECT'
    if (yandex_query_enabled or web_sql_enabled) and is_testing:
        external_project_ids_with_action[YANDEX_QUERY_PREPROD_PROJECT_ID] = 'ACCEPT'
    elif is_porto and reject_enabled:
        external_project_ids_with_action[YANDEX_QUERY_PREPROD_PROJECT_ID] = 'REJECT'

    if data_lens_enabled:
        external_project_ids_with_action['0x453e'] = 'ACCEPT'
        external_project_ids_with_action['0xf83d'] = 'ACCEPT'
    elif is_porto and reject_enabled:
        external_project_ids_with_action['0x453e'] = 'REJECT'
        external_project_ids_with_action['0xf83d'] = 'REJECT'
    if data_lens_enabled and is_testing:
        external_project_ids_with_action['0x453d'] = 'ACCEPT'
        external_project_ids_with_action['0xfc1f'] = 'ACCEPT'
    elif is_porto and reject_enabled:
        external_project_ids_with_action['0x453d'] = 'REJECT'
        external_project_ids_with_action['0xfc1f'] = 'REJECT'

    if metrika_enabled:
        external_project_ids_with_action['0x4b9c'] = 'ACCEPT'
        external_project_ids_with_action['0x660'] = 'ACCEPT'
    elif is_porto and reject_enabled:
        external_project_ids_with_action['0x4b9c'] = 'REJECT'
        external_project_ids_with_action['0x660'] = 'REJECT'

    if serverless_enabled:
        external_project_ids_with_action['0xf82f'] = 'ACCEPT'
    elif is_porto and reject_enabled:
        external_project_ids_with_action['0xf82f'] = 'REJECT'
    if serverless_enabled and is_testing:
        external_project_ids_with_action['0xfc15'] = 'ACCEPT'
    elif is_porto and reject_enabled:
        external_project_ids_with_action['0xfc15'] = 'REJECT'

    if data_transfer_enabled:
        external_project_ids_with_action[DATA_TRANSFER_PROD_PROJECT_ID] = 'ACCEPT'
    elif is_porto and reject_enabled:
        external_project_ids_with_action[DATA_TRANSFER_PROD_PROJECT_ID] = 'REJECT'
    if data_transfer_enabled and is_testing:
        external_project_ids_with_action[DATA_TRANSFER_PREPROD_PROJECT_ID] = 'ACCEPT'
    elif is_porto and reject_enabled:
        external_project_ids_with_action[DATA_TRANSFER_PREPROD_PROJECT_ID] = 'REJECT'

    if exclude_user_project_id:
        try:
            del external_project_ids_with_action[user_project_id]
        except KeyError:
            pass

    return external_project_ids_with_action


def is_in_user_project_id():
    return _pillar('data:is_in_user_project_id', False)


def ip6_porto_user():
    return ip6_porto_user_addr()


def ip6_porto_user_addr():
    porto_ip = _grains('porto_resources:container:ip')
    porto_ips = porto_ip.split(';')
    return porto_ips[0][5:]


def ip6_porto_control():
    return ip6_porto_control_addr()


def ip6_porto_control_addr():
    porto_ip = _grains('porto_resources:container:ip')
    porto_ips = porto_ip.split(';')
    if len(porto_ips) == 2:
        return porto_ips[1][5:]
    return porto_ips[0][5:]


def _pillar(key, default=None):
    return __salt__['pillar.get'](key, default=default)


def _grains(key, default=None):
    return __salt__['grains.get'](key, default=default)
