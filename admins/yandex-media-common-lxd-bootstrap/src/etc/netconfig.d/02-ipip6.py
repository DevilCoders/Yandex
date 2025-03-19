from __builtin__ import filter as _filter
import sys
sys.path.append('/usr/lib/yandex/python-netconfig/')
from interfaces.generators import TunnelIfaceGenerator
from interfaces.network_info import NetworkInfo
from interfaces.conductor import Conductor
from interfaces.interface_utils import InterfaceUtils
from interfaces.interface import Interface
import yaml


CONDUCTOR_ENV = 'production'
MAP64_CONFIG = '/etc/yandex/map64.yaml'


class map64TunnelIfaceGenerator(TunnelIfaceGenerator):
    """ Patching TunnelIfaceGenerator not to use ipv4 in dns, only config """
    def generate_ip4tunnel(self, parameters, iface_utils, network_info, interfaces_group, map64):
        ip = network_info.get_ip_addresses(parameters.fqdn)
        if not ip['ip6']:
          return
        ip6 = ip['ip6']
        ip4tunnel = map64.get(parameters.fqdn, None)
        if not ip4tunnel:
          return

        iface = Interface(network_info)
        iface.mtu = 1450
        iface.do_default = False
        iface.do_ethtool = False
        iface.name = 'ip6tun0'
        iface.itype = 'manual'
        iface.add_up('modprobe ip6_tunnel || true')
        iface.add_up('ifconfig ip6tun0 up mtu 1450')
        iface.add_down('ifconfig ip6tun0 down')
        iface.add_preup('ip -6 tunnel add ip6tun0 mode ipip6 remote 2a02:6b8:b010:a0ff::1 local %s' % ip6)
        iface.add_postdown('ip -6 tunnel del ip6tun0 mode ipip6 remote 2a02:6b8:b010:a0ff::1 local %s' % ip6)
        iface.add_postup('ip address add %s/32 dev ip6tun0' % ip4tunnel)
        iface.add_postdown('ip address del %s/32 dev ip6tun0' % ip4tunnel)
        iface.add_postup('ip route add 0/0 dev ip6tun0 mtu 1450')
        iface.add_postdown('ip route del 0/0 dev ip6tun0 mtu 1450')
        interfaces_group.add_interface(iface)


def process(interfaces, params):
    with open(MAP64_CONFIG) as f:
        map64 = yaml.load(f)
    conductor = Conductor(CONDUCTOR_ENV, None)
    map64Generator = map64TunnelIfaceGenerator()
    network_info = NetworkInfo(conductor, None)
    iface_utils = InterfaceUtils(network_info)
    map64Generator.generate_ip4tunnel(params, iface_utils, network_info, interfaces, map64)

def filter(params):
    if 'ipip6' in params.host_tags:
        return True
    return False
