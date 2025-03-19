import re
from ..interface import Interface
from ..bridge_interface import BridgeInterface


class VlanIfaceGenerator(object):
    def generate(self, parameters, iface_utils, network_info, interfaces):
        vlans = [re.sub(r'vlan(\d+)', r'\1', t) for t in parameters.host_tags if re.match(r'vlan\d+', t)]
        vlans = list(set(vlans))

        for vlan in vlans:
            d_iface = parameters.default_iface
            eth_name = '%s.%s' % (d_iface, vlan)

            tagged_eth = Interface(network_info)
            tagged_eth.auto = True
            tagged_eth.name = eth_name
            tagged_eth.itype = 'manual'
            tagged_eth.do_default = False
            tagged_eth.do_ethtool = False
            tagged_eth.vlan_raw_device = d_iface
            tagged_eth.add_preup('ebtables -t broute -A BROUTING -i %s -p 802_1Q -j DROP' % d_iface)
            interfaces.add_interface(tagged_eth)

            tagged_br = BridgeInterface(network_info)
            tagged_br.auto = True
            tagged_br.name = 'br0.%s' % vlan
            tagged_br.bridge_ports = eth_name
            tagged_br.bridge_maxwait = 0
            tagged_br.bridge_fd = 0
            tagged_br.mtu = parameters.mtu
            tagged_br.add_preup('ifconfig %s mtu %d ; true' % (d_iface, tagged_br.mtu))
            tagged_br.add_preup('ifconfig %s mtu %d ; /bin/true' % (eth_name, tagged_br.mtu))
            tagged_br.add_up('ifconfig vlan%s mtu %d ; /bin/true' % (vlan, tagged_br.mtu))
            interfaces.add_interface(tagged_br)
