#!/skynet/python/bin/python
# coding: utf8
import re
import logging
import collections

import transliterate

from aux_http_api import HttpApi
from aux_network import get_first_network_address

REPLACE_CHARS = re.compile('[^a-zA-Z0-9-._]')


class OopsApi(HttpApi):
    API_URL_PRODUCTION = 'https://oops.yandex-team.ru/api'
    BATCH_SIZE = 5000

    def __init__(self, oauth_token=None):
        super(OopsApi, self).__init__(self.API_URL_PRODUCTION, oauth_token)
        self.base_keys_to_fetch = ['bot_status', 'location', 'net_info', 'cpu_info', 'mem_info', 'disks', 'gpu_info']

    def all_fqdns(self):
        return self._get('/hosts/fqdn')

    def hosts_attributes(self, keys, hosts):
        cgi_data = {'key': keys, 'fqdn': list(set(hosts))}
        return self._post('/hosts/attributes', cgi_data=cgi_data)

    def attributes(self, hosts=None, keys=None):
        keys = keys or self.base_keys_to_fetch
        hosts_left = list(set(hosts)) if hosts else self.all_fqdns()

        result = {}
        while hosts_left:
            batch_hosts, hosts_left = hosts_left[:self.BATCH_SIZE], hosts_left[self.BATCH_SIZE:]
            batch_response = self.hosts_attributes(keys, batch_hosts)

            batch_result = collections.defaultdict(dict)
            for host_data in batch_response:
                hostname = str(host_data.get('fqdn'))

                attribute_key = host_data.get('attributeKeyName')
                if attribute_key is None:
                    continue

                attribute = host_data.get('attribute')
                if attribute is None:
                    continue

                attribute_change_time = self._handler_change_time(attribute)
                batch_result[hostname][attribute_key] = self._handler_attribute(attribute)
                batch_result[hostname][attribute_key].update({'@changeTime': attribute_change_time})
            result.update(batch_result)

        return result

    @staticmethod
    def _handler_change_time(attribute):
        attribute_change_time = None
        if attribute.get('changeTime') is not None:
            attribute_change_time = attribute['changeTime'].split('.')[0]
        elif attribute.get('checkTime') is not None:
            attribute_change_time = attribute['checkTime'].split('.')[0]
        return attribute_change_time

    @staticmethod
    def _handler_attribute(attribute):
        info = {k: v for k, v in attribute.items()
                if k not in ('changeTime', 'checkTime')}
        return info


def parse_bot_status(bot_status_attribute, hostname):
    bot_status_info = {}
    bot_status_info['invnum'] = str(bot_status_attribute['id'])
    bot_status_info['botprj'] = str(transliterate.translit(
        bot_status_attribute['project'], 'ru', reversed=True
    ).encode('utf-8').replace('"', ''))
    return bot_status_info


def parse_location(location_attribute, hostname):
    dc = location_attribute['city'].lower()
    building = location_attribute['building'].lower()
    queue = location_attribute['line'].lower()

    # detecting dc is more difficult
    if dc == 'mantsala':
        dc = 'man'
    elif dc == 'mow':
        dc = building
    elif dc == 'vladimir':
        dc = 'vla'

    # detecting queue is more difficult
    if queue == 'man-2#b.1.07':
        queue = 'man2'
    elif queue == 'man-1#b.1.06':
        queue = 'man1'

    return {
        'dc': str(REPLACE_CHARS.sub('_', dc)),
        'queue': str(REPLACE_CHARS.sub('_', queue)),
        'rack': str(location_attribute['rack']),
        'unit': str(location_attribute['unit']),
    }


def parse_net_info(net_info_attribute, hostname, vlans_to_parse=(688, 788, 721), base_bandwidth=1000):
    interfaces = net_info_attribute.get('interfaces')
    if interfaces is None:
        return None

    vlan2ip = {}
    max_bandwidth = base_bandwidth
    for interface in interfaces:
        name = interface.get('name', '')
        vlan = interface.get('vlan')
        ips = interface.get('ips')
        bandwidth = interface.get('bandwidth')

        if name.startswith('eth') and bandwidth is not None:
            max_bandwidth = max(bandwidth, max_bandwidth)

        if vlan not in vlans_to_parse:
            continue
        elif not vlan or not ips or len(ips) > 1:
            logging.warning('Invalid data for host {} (vlan={}, ips={})'.format(hostname, vlan, ips))
            continue

        vlan2ip['vlan{}'.format(vlan)] = get_first_network_address(ips[0])

    return {'vlans': vlan2ip, 'net': max_bandwidth}


def parse_cpu_info(cpu_info_attribute, hostname):
    return {
        'model': str(cpu_info_attribute.get('processorName')),
    }


def parse_mem_info(mem_info_attribute, hostname):
    botmem = int(mem_info_attribute['capacity'] / 1024. / 1024. / 1024.)
    return {
        'botmem': botmem,
    }


def parse_disks(disks_attribute, hostname):
    disks_info = {
        'botdisk': 0,
        'botssd': 0,
        'botdisk_fs': 0,
        'botssd_fs': 0,
    }

    if not disks_attribute.get('disks'):
        return disks_info

    for disk in disks_attribute.get('disks'):
        disk_type = disk.get('type', '').lower()
        disk_key = None
        if disk_type == 'hdd':
            disk_key = 'botdisk'
        elif disk_type == 'ssd':
            disk_key = 'botssd'

        if disk_key is not None:
            disks_info[disk_key] += disk.get('size', 0)
            disks_info['{}_fs'.format(disk_key)] += disk.get('fsSize', 0)

    for disk_key in disks_info:
        disks_info[disk_key] = int(disks_info[disk_key] / 1024. / 1024. / 1024.)

    return disks_info


def parse_gpu_info(gpu_info_attribute, hostname):
    gpu_info = {'gpu_count': 0, 'gpu_models': []}
    if not gpu_info_attribute.get('adapters'):
        return gpu_info

    for adapter in gpu_info_attribute.get('adapters'):
        name = str(adapter.get('name'))
        memory_size = adapter.get('memorySize')

        gpu_info['gpu_count'] += 1
        if name not in gpu_info['gpu_models']:
            gpu_info['gpu_models'].append(name)
    return gpu_info


if __name__ == '__main__':
    oops = OopsApi()
    all_fqdns = oops.all_fqdns()
    print(json.dumps(oops.hardware(all_fqdns[:6000]), indent=4, sort_keys=True))
