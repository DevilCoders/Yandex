#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import xmltodict
import os

from ncclient import manager
from socket import gaierror
from ncclient.transport.errors import SSHError, AuthenticationError
from ncclient.operations.errors import TimeoutExpiredError
from ncclient.operations.rpc import RPCError
from jinja2 import Environment, FileSystemLoader

from . import args
# import logging
# import sys
# logging.basicConfig(level=logging.DEBUG,
#                     format='%(asctime)s %(levelname)s: %(message)s',
#                     stream=sys.stdout)

curr_dir = os.path.dirname(os.path.abspath(__file__))

env = Environment(
    loader=FileSystemLoader(curr_dir + '/templates'),
    trim_blocks=True,
    lstrip_blocks=True
)


def validate_vlans(valid_vlans, vlan_str):
    vlan_list = []
    vlans = vlan_str.split(',')
    for el in vlans:
        if '-' not in el:
            vlan_list.append(el)
        else:
            vlan_range = el.split('-')
            num_list = list(range(int(vlan_range[0]), int(vlan_range[1]) + 1))
            vlan_list.extend([str(vlan) for vlan in num_list])
    return ','.join(vlan_list) == valid_vlans


def get_template(vendor, operation):
    return env.get_template('{}_intf_{}.j2'.format(vendor, operation))


class Toggler:
    def __init__(self, device_data):
        self.vars = device_data
        self._conn = None

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        if self._conn:
            self._conn.close_session()

    def _create_conn(self):
        try:
            self._conn = manager.connect(
                host=self.vars['tor'],
                port=22,
                timeout=10,
                username=args.user,
                device_params={'name': 'huaweiyang'},
                key_filename=args.key,
                hostkey_verify=False
            )
        except gaierror:
            code, status = 404, 'Failed to resolve ToR fqdn'
        except SSHError:
            code, status = 503, 'Failed to connect to ToR'
        except AuthenticationError:
            code, status = 403, 'Failed to authenticate on ToR'

        if self._conn is None:
            return {
                'status': status,
                'code': code
            }

    @property
    def toggle(self):
        operation = 'edit'
        conn_result = self._create_conn()
        if isinstance(conn_result, dict):
            result = conn_result
        else:
            payload = get_template(self.vars['device_type'], operation).render(self.vars)
            try:
                self._conn.edit_config(payload, target='running')
            except TimeoutExpiredError:
                result = {
                    'status': 'Timeout for RPC reply has expired',
                    'code': 408
                }
            except RPCError:
                result = {
                    'status': 'Invalid RPC request',
                    'code': 400
                }
            else:
                result = {
                    'tor': self.vars['tor'],
                    'intf': self.vars['intf'],
                    'state': self.vars['state'],
                    'status': 'ok',
                    'code': 200
                }

        return result

    @property
    def check(self):
        operation = 'get'
        conn_result = self._create_conn()

        if isinstance(conn_result, dict):
            result = conn_result
        else:
            payload = get_template(self.vars['device_type'], operation).render(self.vars)
            try:
                response = self._conn.get(('subtree', payload))
            except TimeoutExpiredError:
                result = {
                    'status': 'Timeout for rpc-reply has expired',
                    'code': 408
                }
            except RPCError:
                result = {
                    'status': 'Invalid RPC request',
                    'code': 400
                }

            else:
                data = xmltodict.parse(response.data_xml)['data']['ethernet']['ethernetIfs']['ethernetIf']
                if data['l2Enable'] == 'enable':
                    l2_data = data['l2Attribute']
                    mode = l2_data['linkType']
                    native = l2_data['pvid']
                    if mode == 'access' \
                            and native == self.vars['setup_vlan']:
                        state = 'setup'
                    elif 'trunkVlans' in l2_data \
                            and native == self.vars['untagged_vlan'] \
                            and validate_vlans(self.vars['tagged_vlans'], l2_data['trunkVlans']):
                        state = 'prod'
                    else:
                        state = 'unknown'
                else:
                    state = 'l3'

                if state == 'unknown':
                    state += ': mode {}, native vlan {}'.format(
                        mode,
                        native
                    )
                    if 'trunkVlans' in l2_data:
                        state += ', tagged vlans {}'.format(l2_data['trunkVlans'])

                result = {
                    'tor': self.vars['tor'],
                    'intf': self.vars['intf'].upper(),
                    'state': state,
                    'status': 'ok',
                    'code': 200
                }
        return result
