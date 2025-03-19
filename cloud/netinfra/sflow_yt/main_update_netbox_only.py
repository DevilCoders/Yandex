#!/usr/bin/env python3
# -*- coding=utf-8 -*-
from __future__ import print_function
from __future__ import absolute_import
from __future__ import unicode_literals

import itertools
import ipaddress

import asyncio
import netbox
import logging
import datetime
from pprint import pprint


from yql.api.v1.client import YqlClient
# import yt.yson as yson
import yt.wrapper as yt
import ytit


def init_ll_for_v4():
    ipv4_ll = ('10.110.0.0/16',
               '10.120.0.0/16',
               '10.130.0.0/16',
               '10.140.0.0/16',
               '10.255.254.0/24',
               '10.255.255.0/24')

    net_objects = []
    for net in ipv4_ll:
        net_objects.append(ipaddress.ip_network(net))

    def check_v4_ll(ip):
        for net in net_objects:
            if net.overlaps(ip):
                return True
        return False
    return check_v4_ll


def get_netbox_information():
    logging.info(f"Getting device list")
    nb = netbox.Netbox('https://netbox.cloud.yandex.net/api')
    tors = nb.get_devices_list('role=tor&role=ct&tenant=production&status=1')

    tors = nb.get_devices_location(tors)
    # pprint(tors)
    logging.info(f"Found {len(tors.keys())} tors")

    tors_interfaces_addresses_urls = nb.get_addresses_by_device_names(tors)
    loop = asyncio.get_event_loop()
    tors_interfaces_addresses = loop.run_until_complete(nb.get_bunch(tors_interfaces_addresses_urls))
    # pprint(tors_interfaces_addresses)

    agents = {}
    prefixes = {}
    skipped_v4_ll = 0
    skipped_v6_ll = 0
    skipped_mgmt = 0
    parsed_interfaces = 0
    duplicates = set()

    # init_ll_for_v4 return preinitialized function with network's objects for LL checks
    is_v4_ll = init_ll_for_v4()

    try:
        for x in list(itertools.chain.from_iterable(tors_interfaces_addresses)):
            address = ipaddress.ip_interface(x['address'])
            if address.is_link_local:
                skipped_v6_ll = skipped_v6_ll+1
                continue
            if x['vrf'] is not None:
                if x['vrf']['name'] == 'MGMT':
                    skipped_mgmt = skipped_mgmt+1
                    continue
            if is_v4_ll(address.network) and x['vrf'] is None:
                skipped_v4_ll = skipped_v4_ll + 1
                logging.debug(x['vrf'])
                continue
            if ipaddress.ip_network('169.254.0.0/16').overlaps(address.network):
                skipped_v4_ll = skipped_v4_ll + 1
                continue

            device = str(x['interface']['device']['name'])
            iface = str(x['interface']['name'])
            # print(f"{address.network} {device} {iface}")

            if iface == 'loopback0':
                agents[address.ip.__str__()] = {'name': device}
                # print(x)

            if address.network.__str__() in prefixes.keys():
                print(f"{address.network.__str__()} {device} already in table with device {prefixes[address.network.__str__()]}")
                duplicates.add(address.network.__str__())
                print(x)
            prefixes[address.network.__str__()] = {'name': device}
            parsed_interfaces = parsed_interfaces + 1
    except TypeError as e:
        print(e)
        print(e.args)
        print(tors_interfaces_addresses)
        exit()

    logging.info(f"Parsed {parsed_interfaces} interfaces. Skipped: v6LL {skipped_v6_ll}, " +
                 f"v4LL {skipped_v4_ll}, " +
                 f"MGMT: {skipped_mgmt}.\n" +
                 f"\tTotal parsed {parsed_interfaces+skipped_v6_ll+skipped_v4_ll+skipped_mgmt}.")

    logging.info(f"{len(agents)} agents IP associated with ToR's name.")
    logging.info(f"{len(prefixes.keys())} prefixes created.")

    # pprint(tors)
    # pprint(agents)
    # pprint(prefixes)
    # pprint(duplicates)
    logging.info(f"{len(duplicates)} duplicates.")
    if len(duplicates) != 0:
        logging.error(duplicates)
        logging.error("Please resolve issue with duplicates")
        exit()

    return tors, agents, prefixes


def main():
    logging.basicConfig(format='%(asctime)s %(levelname)s:%(name)s %(message)s', level=logging.INFO)
    logging.getLogger('urllib3').setLevel(logging.WARNING)
    logging.getLogger('requests').setLevel(logging.WARNING)
    logging.getLogger('yt.packages.urllib3.connectionpool').setLevel(logging.WARNING)
    logging.getLogger('urllib3.connectionpool').setLevel(logging.WARNING)
    logging.getLogger('yql.client.request').setLevel(logging.INFO)
    logging.basicConfig(level=logging.DEBUG)

    tors, agents, prefixes = get_netbox_information()

    ytit.update_netbox_tables(tors, agents, prefixes)

    # yesterday = (datetime.date.today() - datetime.timedelta(days=1))
    # yesterday = yesterday.strftime('%Y-%m-%d')

    # if ytit.is_calculation_proceed_required(yesterday):
    #     logging.info('Flow aggregation is required. Starting...')
    #     ytit.process_daily_aggregation(yesterday)

    # ytit.process_daily_aggregation('2019-06-21')


if __name__ == '__main__':
    main()
