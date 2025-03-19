from __builtin__ import filter as _filter
import sys
sys.path.append('/usr/lib/yandex/python-netconfig/')
from interfaces.route import Route
from interfaces.interface import Interface


class FixedFamilyInterface(Interface):
    """
    Just see https://github.yandex-team.ru/devtools/python-netconfig/pull/34
    Remove this when fix would be released
    """

    def __new__(cls, obj):
        if isinstance(obj, Interface):
            obj.__class__ = FixedFamilyInterface
            return obj
        return object.__new__(cls)

    def __init__(self, obj):
        pass

    def generate_actions(self):
        gen_preup = []
        gen_postup = []
        gen_predown = []
        gen_postdown = []

        if self.do_default:
            route_replace = Route().replace().ip_version(self.family).default().via(self.gateway).mtu(self.default_mtu)
            if not self.default_advmss is None: route_replace.advmss(self.default_advmss)
            gen_postup.append(route_replace)

        if self.default_mtu != self.mtu and self.jumboroute:
            if self.use_tables and self.is_fastbone:
                route_replace = Route().replace().ip_version(self.family).default().via(self.gateway).mtu(self.encap_mtu).table(self.vlan)
                gen_postup.append(route_replace)
                for jn in self.jumbo_nets:
                    rule_add = Rule().add().ip_version(self.family).to_addr(jn).lookup(self.vlan)
                    gen_postup.append(rule_add)
            else:
                for jn in self.jumbo_nets:
                    route_replace = Route().replace().ip_version(self.family).route(jn).via(self.gateway).mtu(self.encap_mtu)
                    gen_postup.append(route_replace)

        vz_postup = '[ -e /tmp/#IFACE#veth ] && ( for iface in `cat /tmp/#IFACE#veth`;' + \
                    'do export VEID=`echo $iface | sed -r \'s/^veth([0-9]+)\..*$/\\1/\'`; ' + \
                    '[ -x /usr/bin/vznetaddbr -a x`vzlist -H -o status $VEID` = xrunning ] ' + \
                    '&& /usr/bin/vznetaddbr init veth $iface; done; rm -f /tmp/#IFACE#veth ); true'
        vz_predown = 'brctl show #IFACE# | awk \'$NF ~ /^veth/ { print $NF }\' > /tmp/#IFACE#veth; true'
        ethtool_tune = 'ethtool -K #IFACE# tso off; if lsmod | grep -qE "e1000|igb"; ' + \
                       'then ethtool -G #IFACE# rx 4096 tx 4096; fi; true'

        if self.bridge_ports:
            gen_preup.append("ifconfig " + self.bridge_ports + " mtu " + str(self.mtu) + "; true")
            gen_postup.append(vz_postup.replace("#IFACE#", self.name))
            gen_predown.append(vz_predown.replace("#IFACE#", self.name))
            if self.do_ethtool:
                gen_preup.append(ethtool_tune.replace("#IFACE#", self.bridge_ports))
        else:
            if self.do_ethtool:
                gen_preup.append(ethtool_tune.replace("#IFACE#", self.name))

        if self.family == 6 and self.mtu is not None:
            self.up = ['ifconfig {} up mtu {}; true'.format(self.name, self.mtu)] + self.up
            self.mtu = None

        self.preup = gen_preup + self.preup
        self.postup = gen_postup + self.postup
        self.predown = gen_predown + self.predown
        self.postdown = gen_postdown + self.postdown


""" Module to generate interfaces for host with RA """
def process(interfaces, params):
    """ br for bare metal, eth/lo for virtual """
    to_filter = ['eth0', 'eth1', 'br0']
    for i in interfaces:
        if i.name in to_filter and i.family == 6:
            i = FixedFamilyInterface(i)  # Remove this when fix would be released
            i.itype = 'manual'
            i.network = ''
            i.netmask = ''
            i.address = ''
            i._do_default = False
            route_to_gw = Route().replace().ip_version(i.family).route(i.gateway).dev(i.name)
            if i.encap_mtu:
                i.add_preup(str(route_to_gw.mtu(i.encap_mtu)) + " || true")
            else:
                i.add_preup(str(route_to_gw) + " || true")
            i.add_postup('sysctl -w net.ipv6.conf.%s.ra_default_route_mtu=1450' % (i.name))
            i.add_postup('sysctl -w net.ipv6.conf.%s.use_tempaddr=0' % (i.name))
        elif _filter(lambda x: i.name.startswith('%s.' % x), to_filter):
            i.add_preup('sysctl -w net.ipv6.conf.%s.accept_ra=0; true' % (i.name.replace('.', '/')))

def filter(params):
    """ Run only if host have tag RA in conductor """
    if 'RA' in params.host_tags:
        return True
    return False
