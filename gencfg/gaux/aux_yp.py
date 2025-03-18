#!/skynet/python/bin/python
# coding: utf8

import sys
import os
import mmap
import contextlib
import json
import ipaddress
import socket

from yp.client import YpClient
from yp.common import YpNoSuchObjectError

from yt.yson import YsonEntity

from collections import defaultdict

class YP:
    VLANS_MAP = {
        'fastbone': 'vlan788',
        'backbone': 'vlan688'
    }

    def __init__(self, oauth_token):
        self.yp_client_by_dc = {
            'iva': YpClient('iva.yp.yandex.net:8090', config={"token": oauth_token}),
            'sas': YpClient('sas.yp.yandex.net:8090', config={"token": oauth_token}),
            'man': YpClient('man.yp.yandex.net:8090', config={"token": oauth_token}),
            'vla': YpClient('vla.yp.yandex.net:8090', config={"token": oauth_token}),
            'myt': YpClient('myt.yp.yandex.net:8090', config={"token": oauth_token})
        }
        self.vlans_by_dc = defaultdict(dict)
        self.net_by_dc = defaultdict(dict)
        for key, clt in self.yp_client_by_dc.iteritems():
            self.vlans_by_dc[key].update(self.read_all(clt, 'node', ['/meta/id', '/spec/ip6_subnets'], [0, 1]))
            self.net_by_dc[key].update(self.read_all(clt,
                                                     'resource',
                                                     ['/meta/id', '/meta/node_id', '/spec/network'],
                                                     [1, 2],
                                                     '[/meta/kind] = "network"'))

    def read_all(self, clt, object_type, selectors, indices, custom_filter = "", batch_size=5000):
        result = {}
        last_id = None
        finish_flag = False
        while not finish_flag:
            retries = 10
            error = None
            while retries > 0:
                try:
                    fltr = None
                    if last_id:
                        fltr = ""
                        if custom_filter:
                            fltr = custom_filter + ' AND '
                        fltr += '[/meta/id] > "{}" '.format(last_id)
                    batch = clt.select_objects(object_type, selectors=selectors, filter=fltr, limit=batch_size)
                except Exception as e:
                    retries -= 1
                    error = e
                    continue
                else:
                    if not batch:
                        finish_flag = True
                        break
                    for item in batch:
                        if not isinstance(item[indices[1]], YsonEntity):
                            result[item[indices[0]]] = item[indices[1]]
                    last_id = batch[-1][0]
                    break
            if retries == 0:
                raise error

        return result

    def format_ipv6_address(self, ip_address):
        exploded_ip = str(ip_address.exploded).replace('0000', '0')
        return ':'.join(p.lstrip('0') or '0' for p in exploded_ip.split(':'))

    def get_vlans_for_host(self, host, dc):
        if host not in self.vlans_by_dc[dc]:
           print("Hostname {} has no vlans info in YP".format(host))
           return {}
        data = self.vlans_by_dc[dc][host]
        vlans = {}
        for item in data:
            ip = ipaddress.IPv6Network(unicode(item['subnet']))
            vlans[self.VLANS_MAP[item['vlan_id']]] = self.format_ipv6_address(ip[1])
        return vlans

    def get_net_for_host(self, host, dc):
        if host not in self.net_by_dc[dc]:
           print("Hostname {} has no net info in YP".format(host))
           return 0
        data = self.net_by_dc[dc][host]
        return data['total_bandwidth'] * 8 / 1000 / 1000

    def parse_net_info_for_host(self, host, dc):
        vlans = self.get_vlans_for_host(host, dc)
        net = self.get_net_for_host(host, dc)
        if not vlans and not net:
            return {}
        elif not vlans:
            return {'net': net}
        elif not net:
            return {'vlans': vlans}
        else:
            return {'vlans': vlans, 'net': net}
