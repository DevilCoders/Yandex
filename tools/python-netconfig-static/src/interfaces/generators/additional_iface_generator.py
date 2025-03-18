
import re
import hadoop_generator
from ..bridge_interface import BridgeInterface
from ..interface import Interface
from ..rule import Rule
from ..address import Address


class AdditionalIfaceGenerator(object):
    def generate(self, parameters, iface_utils, network_info, interfaces_group):
        # Copy-paste from PlainIfaceGenerator
        # >
        iface_params = {
            'name': parameters.default_iface,
            'mtu': parameters.mtu,
            'do_ethtool': not parameters.virtual_host,
        }
        if 'heavy-traffic' in parameters.host_tags:
            iface_params['postup'] = [
                "/sbin/iptables -t mangle -A OUTPUT -p tcp -m multiport --sport 80,443 -j DSCP --set-dscp 0x08"]

        if 'disk-heavy-traffic' in parameters.host_tags:
            iface_params['postup'] = [
                "/sbin/iptables -t mangle -A OUTPUT -p tcp -m multiport --sport 80,443,10010 -j DSCP --set-dscp 0x08"]

        if parameters.multiqueue:
            iface_params['preup'] = [
                'ethtool -L ' + iface_params['name'] + ' combined $(nproc) 2>/dev/null || true']

        if parameters.bridged:
            iface_params['bridge_ports'] = iface_params['name']
            iface_params['name'] = 'br0'
        # <

        if 's3-metadata-iface' in parameters.host_tags:
            iface = {}
            iface['name'] = 'lo:400'
            iface['itype'] = 'manual'
            iface['do_default'] = False
            iface['do_ethtool'] = False
            postup = []
            predown = []
            postup.append(Address().add().address(
                '169.254.169.254/32').dev('lo').label('lo:400'))
            postup.append(Rule().add().from_addr(
                '169.254.169.254/32').lookup('400'))
            predown.append(Address().delete().address(
                '169.254.169.254/32').dev('lo').label('lo:400'))
            predown.append(Rule().delete().from_addr(
                '169.254.169.254/32').lookup('400'))
            iface['postup'] = postup
            iface['predown'] = predown
            interfaces_group.add_interface(
                Interface.from_params(network_info, iface))

        if 'hdp_tundra' in parameters.host_groups:
            iface = {}
            iface['name'] = parameters.default_iface+':1'
            iface['address'] = hadoop_generator.internalHDPIP(parameters.fqdn)
            iface['netmask'] = '255.255.0.0'
            iface['do_default'] = False
            iface['mtu'] = 8950
            iface['do_ethtool'] = False
            interfaces_group.add_interface(
                Interface.from_params(network_info, iface))

        if 'narod-misc-dom0' in parameters.host_groups:
            default_narod_iface = 'eth1'
            if parameters.default_iface == 'eth1':
                default_narod_iface = 'eth0'
            narod_misc_dom0_iface = parameters.narod_misc_dom0_iface or default_narod_iface

            bridge_iface = BridgeInterface(network_info)
            bridge_iface.name = 'br528'
            bridge_iface.mtu = 8950
            bridge_iface.bridge_ports = narod_misc_dom0_iface
            bridge_iface.add_preup("ifconfig %s mtu %d; true" %
                                   (narod_misc_dom0_iface, 8950))

            interfaces_group.add_interface(bridge_iface)

        if 'n2stor' in parameters.host_groups:
            facename = parameters.fqdn.replace('n2stor', 'face2n')
            iface_params['do_default'] = False
            iface_params['do_ethtool'] = False
            iface_params['name'] += ':1'
            interfaces_group.add_interfaces(
                iface_utils.generate_plain_interface(facename, iface_params))

        if 'naroddisk_storages' in parameters.host_groups:
            number = None
            host_pattern = None
            if 'naroddisk-regional-storages' in parameters.host_groups:
                if 'naroddisk-regional-storages-fast' in parameters.host_groups:
                    m = re.search(
                        r'filestore-(\w+)\.narod\.yandex\.net', parameters.fqdn)
                    number = 1
                    city = m.group(1)
                    host_pattern = '-filestore-'+city+'-narod.yandex.ru'
                elif 'naroddisk-regional-storages-slow' in parameters.host_groups:
                    m = re.search(
                        r'filestore-(\w+)-sata(\d+)?\.narod\.yandex\.net', parameters.fqdn)
                    city = m.group(1)
                    number = m.group(2) and m.group(2) or 1
                    # up1s-filestore-kiev-narod.yandex.ru
                    host_pattern = 's-filestore-' + city + '-narod.yandex.ru'
            elif 'naroddisk_storages_fast' in parameters.host_groups:
                m = re.search(
                    r'filestore(\d+)(\w+)\.narod\.yandex\.net', parameters.fqdn)
                number = m.group(1)
                dc_letter = m.group(2)
                host_pattern = '' + dc_letter + '-narod.yandex.ru'
            elif 'naroddisk_storages_slow' in parameters.host_groups:
                m = re.search(
                    r'filestore-sata-(\d+)(\w+)\.narod.yandex.net', parameters.fqdn)
                number = m.group(1)
                dc_letter = m.group(2)
                host_pattern = 's' + dc_letter + '-narod.yandex.ru'

            if number and host_pattern:
                dl_ip = network_info.get_host_ip(
                    'dl' + str(number) + host_pattern)
                iface = {}
                iface['comments'] = 'dl' + str(number) + host_pattern
                iface['name'] = parameters.default_iface + ':1'
                iface['address'] = dl_ip
                iface['netmask'] = '255.255.255.255'
                iface['do_default'] = False
                iface['mtu'] = None
                iface['do_ethtool'] = False
                interfaces_group.add_interface(
                    Interface.from_params(network_info, iface))

                up_ip = network_info.get_host_ip(
                    'up' + str(number) + host_pattern)
                iface = {}
                iface['comments'] = 'up' + str(number) + host_pattern
                iface['name'] = parameters.default_iface + ':2'
                iface['address'] = up_ip
                iface['netmask'] = '255.255.255.255'
                iface['do_default'] = False
                iface['mtu'] = None
                iface['do_ethtool'] = False
                interfaces_group.add_interface(
                    Interface.from_params(network_info, iface))
