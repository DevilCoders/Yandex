#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import pynetbox
# import logging
# import sys
# logging.basicConfig(level=logging.DEBUG,
#                     format='%(asctime)s %(levelname)s: %(message)s',
#                     stream=sys.stdout)

netbox_url = 'http://netbox.cloud.yandex.net'


class NetboxInterface:
    def __init__(self, fqdn, intf, state=None, url=netbox_url):
        self.fqdn = fqdn.split('.')[0]
        self.intf = intf
        if state is not None:
            self.state = state
        self.url = url
        self._conn = None
        self.interface_data = self._get_device_data()

    def _get_device_data(self):
        try:
            self._conn = pynetbox.api(self.url)
        except OSError:
            code, status = 503, 'Failed to connect to netbox'
        else:
            self._nb_device = self._conn.dcim.devices.get(name='{}.netinfra.cloud.yandex.net'.format(self.fqdn), role=['tor', 'ct'], status=1)
            if self._nb_device is None:
                self._nb_device = self._conn.dcim.devices.get(name='{}.yndx.net'.format(self.fqdn), role=['tor', 'ct'], status=1)

            if self._nb_device is None:
                code, status = 404, 'There is no such ToR'
            else:
                self.device_type = self._nb_device.device_type.manufacturer.slug
                self._nb_intf = self._conn.dcim.interfaces.get(name=self.intf, device_id=self._nb_device.id)
                if not self._nb_intf:
                    self._nb_intf = self._conn.dcim.interfaces.get(name=self.intf.upper(), device_id=self._nb_device.id)

                if not self._nb_intf:
                    code, status = 404, 'There is no such interface on this ToR'
                elif self._nb_intf.description != 'Downlink':
                    code, status = 403, 'You are not allowed to switch this interface'
                else:
                    setup_vlans_list = self._conn.ipam.vlans.filter(
                        site_id=self._nb_device.site.id,
                        role="setup"
                    )
                    untagged_vlan = self._nb_intf.untagged_vlan
                    tagged_vlans = self._nb_intf.tagged_vlans
                    if not setup_vlans_list and self.state == 'setup':
                        code, status = 404, 'There is no SETUP vlan for this ToR'
                    elif not untagged_vlan:
                        code, status = 404, 'There is no NATIVE VLAN for this interface'
                    elif not tagged_vlans:
                        code, status = 404, 'There is no TAGGED VLANS for this interface'
                    else:
                        self.setup_vlan = str(setup_vlans_list[0].vid)
                        self.untagged_vlan = str(untagged_vlan.vid)
                        self.tagged_vlans = [str(vlan.vid) for vlan in tagged_vlans]
                        return {
                            'code': 200,
                            'tor': self._nb_device.name,
                            'intf': self._nb_intf.name,
                            'device_type': self.device_type,
                            'setup_vlan': self.setup_vlan,
                            'untagged_vlan': self.untagged_vlan,
                            'tagged_vlans': ','.join(self.tagged_vlans)
                        }

        return {
            'code': code,
            'status': status,
        }
