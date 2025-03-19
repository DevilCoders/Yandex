import re
from ..interface import Interface
from ..utils.local import local_address


class FastboneIfaceGenerator(object):
    fastbone_vlans = []

    def generate(self, parameters, iface_utils, network_info, interfaces_group):
        fastbone_params = dict(
            iface=parameters.default_iface,
            main_fqdn=parameters.fqdn,
            mtu=parameters.mtu,
            tagged_vlan=True,
            do_ethtool=not parameters.virtual_host,
            use_tables=parameters.use_tables,
        )
        if parameters.fb_iface:
            fastbone_params['iface'] = parameters.fb_iface
            fastbone_params['tagged_vlan'] = False

        if 'bonding' in parameters.host_tags and parameters.bonding:
            fastbone_params['iface'] = 'bond0'

        fastbone_fqdns = [
            'fb.' + parameters.fqdn,
            'fb-' + parameters.fqdn,
            re.sub(r'yandex\.(ru|net)', r'fb.\g<0>', parameters.fqdn),
            re.sub(r'(market|vertis)\.yandex\.net', r'fb.\g<0>', parameters.fqdn),
            re.sub(r'\.storage\.yandex\.net', r'-fb\g<0>', parameters.fqdn),
            'fastbone.' + parameters.fqdn,
            'bs.' + parameters.fqdn,
            'tank.' + parameters.fqdn,
        ]

        main_ips = network_info.get_ip_addresses(parameters.fqdn)
        if not main_ips:
            raise ValueError('cannot resolve fqdn to any ip address')

        fb_ifaces = None
        for fb_fqdn in fastbone_fqdns:
            fastbone_params['fb_fqdn'] = fb_fqdn
            fb_ifaces = self.generate_fastbone_interface(network_info, fastbone_params, main_ips)
            if fb_ifaces:
                break

        if fb_ifaces:
            interfaces_group.add_interfaces(fb_ifaces)
            if parameters.fb_bridged:
                fb_bridge = Interface(network_info)
                fb_bridge.auto = True
                fb_bridge.name = 'fb0'
                fb_bridge.itype = 'manual'
                fb_bridge.mtu = parameters.mtu
                fb_bridge.render_bridge_params = True
                fb_bridge.do_default = False
                fb_bridge.do_ethtool = False
                fb_bridge.bridge_ports = fb_ifaces[0].name
                interfaces_group.add_interface(fb_bridge)

            main_ipv6 = filter(
                lambda i: i.name == parameters.default_iface and i.address == main_ips['ip6'], interfaces_group
            )
            if main_ipv6 and self.fastbone_vlans:
                main_ipv6[0].ya_netcfg_fb_iface = self.fastbone_vlans[0]

    def generate_fastbone_interface(self, network_info, opts, main_ips):
        if not opts['tagged_vlan']:
            opts['name'] = opts['iface']

        if not opts['iface']:
            raise ValueError(':iface should be specified')
        if not opts['main_fqdn']:
            raise ValueError(':main_fqdn should be specified')
        if not opts['fb_fqdn']:
            raise ValueError(':fb_fqdn should be specified')

        opts['do_default'] = False
        opts['is_fastbone'] = True
        add_ips = network_info.get_all_ip_addresses(opts['fb_fqdn'])
        if not add_ips:
            return False

        add_ips = filter(lambda (f, ip): ip != main_ips['ip4'] and ip != main_ips['ip6'], add_ips)
        if not add_ips:
            return False

        result = []
        auto = []
        for additional_ip in add_ips:
            dummy = {'ip4': None, 'ip6': None, 'ip%d' % additional_ip[0]: additional_ip[1]}
            nets = network_info.get_networks(dummy)
            current_opts = opts.copy()

            iface = local_address(additional_ip[1])
            if iface and not iface.startswith('vlan'):
                current_opts['name'] = iface
                current_opts['tagged_vlan'] = False

            if nets['net4']:
                current_opts.update(nets['net4'])
                if current_opts['tagged_vlan']:
                    current_opts['name'] = 'vlan' + nets['net4']['vlan']
                    current_opts['vlan'] = nets['net4']['vlan']
                else:
                    self.fastbone_vlans.append(('vlan' + nets['net4']['vlan'], current_opts['name']))

            if nets['net6']:
                current_opts.update(nets['net6'])
                if current_opts['tagged_vlan']:
                    current_opts['name'] = 'vlan' + nets['net6']['vlan']
                    current_opts['vlan'] = nets['net6']['vlan']
                else:
                    self.fastbone_vlans.append(('vlan' + nets['net6']['vlan'], current_opts['name']))

            if current_opts['name'] in auto:
                current_opts['auto'] = False

            auto.append(current_opts['name'])

            if current_opts['tagged_vlan']:
                current_opts['vlan_raw_device'] = opts['iface']
                current_opts['preup'] = [
                    '/sbin/ifconfig %s mtu %d' % (current_opts['vlan_raw_device'], current_opts['mtu'])
                ]
                current_opts['do_ethtool'] = False

            result.append(Interface.from_params(network_info, current_opts))

        if not result:
            return False

        return result
