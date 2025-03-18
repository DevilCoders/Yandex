from ..interface import Interface
import platform


class OpenstackIfaceGenerator(object):
    """https://st.yandex-team.ru/CLOUDSRE-187"""

    def generate(self, parameters, iface_utils, network_info, interfaces):
        if 'openstack' not in parameters.host_tags:
            return

        def create_v4_iface(name):
            v4_interface = {}
            v4_interface['name'] = name
            v4_interface['family'] = 4
            v4_interface['preup'] = preup_v4
            v4_interface['up'] = up_v4
            v4_interface['predown'] = predown_v4
            v4_interface['down'] = down_v4
            v4_interface['itype'] = 'manual'
            v4_interface['do_default'] = False
            v4_interface['do_ethtool'] = False
            interfaces.add_interface(
                Interface.from_params(network_info, v4_interface))

        def create_v6_iface(name):
            v6_interface = {}
            v6_interface['name'] = name
            v6_interface['auto'] = False
            v6_interface['family'] = 6
            v6_interface['privext'] = 0
            v6_interface['preup'] = preup_v6
            v6_interface['up'] = up_v6
            v6_interface['down'] = down_v6
            v6_interface['itype'] = 'manual'
            v6_interface['do_default'] = False
            v6_interface['do_ethtool'] = False
            interfaces.add_interface(
                Interface.from_params(network_info, v6_interface))

        interfaces_name = [
            interface.name for interface in interfaces if 'eth' in interface.name]
        for interface in interfaces:
            if interface.name.startswith("lo"):
                continue
            if interface.name.startswith("eth"):
                interfaces.remove_interface(interface)

        linux_release = platform.linux_distribution()[2]

        if linux_release == 'lucid':
            preup_v4 = ['avahi-autoipd -D $IFACE:avahi',
                        'modprobe ipv6 || true',
                        'sysctl net.ipv6.conf.$IFACE.dad_transmits=0 || true']

            up_v4 = ['dhclient3 -nw -e IF_METRIC=100 -pf /var/run/dhclient.$IFACE.pid -lf '
                     '/var/lib/dhcp3/dhclient.$IFACE.leases $IFACE',
                     'ip -6 route add 2a02:6b8::/32 via fe80::1 dev $IFACE mtu 8950 metric 2049',
                     'ip -6 route add 2620:10f:d000::/44 via fe80::1 dev $IFACE mtu 8950 metric 2049']

            predown_v4 = ['avahi-autoipd --kill $IFACE:avahi']

            down_v4 = [
                'dhclient3 -r -pf /var/run/dhclient.$IFACE.pid -lf /var/lib/dhcp3/dhclient.$IFACE.leases $IFACE']

            for name in interfaces_name:
                create_v4_iface(name)

        else:
            preup_v4 = ['ip link set mtu 8950 dev $IFACE || true',
                        '/usr/sbin/avahi-autoipd -D $IFACE:avahi || true']

            predown_v4 = [
                '/usr/sbin/avahi-autoipd --kill $IFACE:avahi || true']

            up_v4 = [
                'dhclient -nw -v -pf /run/dhclient.$IFACE.pid -lf /var/lib/dhcp/dhclient.$IFACE.leases $IFACE']

            down_v4 = [
                'dhclient -r -pf /run/dhclient.$IFACE.pid -lf /var/lib/dhcp/dhclient.$IFACE.leases $IFACE']

            preup_v6 = ['sysctl net.ipv6.conf.$IFACE.accept_ra_rt_info_max_plen = 64 || true',
                        'sysctl net.ipv6.conf.$IFACE.ra_default_route_mtu=1450 || true']

            # https://st.yandex-team.ru/NOCREQUESTS-1453
            up_v6 = ['sysctl -q -e -w net.ipv6.conf.$IFACE.autoconf=0',
                     'ip -6 addr flush dev $IFACE scope global dynamic',
                     'dhclient -6 -nw -D LL -pf /run/dhclient6.$IFACE.pid -lf /var/lib/dhcp/dhclient6.$IFACE.leases $IFACE',
                     'ip -6 route add 2a02:6b8::/32 via fe80::1 dev $IFACE mtu 8950 metric 2049',
                     'ip -6 route add 2620:10f:d000::/44 via fe80::1 dev $IFACE mtu 8950 metric 2049']

            down_v6 = [
                'dhclient -6 -r -D LL -pf /run/dhclient6.$IFACE.pid -lf /var/lib/dhcp/dhclient6.$IFACE.leases $IFACE']

            for name in interfaces_name:
                create_v4_iface(name)
                create_v6_iface(name)
