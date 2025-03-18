from ..address import Address
from ..route import Route
from ..rule import Rule
from ..interface import Interface
from ..tunnel_interface import TunnelInterface
from ..loopback_interface import LoopbackInterface


class TunnelIfaceGenerator(object):
    def generate(self, parameters, iface_utils, network_info, interfaces_group):
        if 'ipip6' in parameters.host_tags:
            self.generate_ip4tunnel(
                parameters, iface_utils, network_info, interfaces_group)

        if 'ipvs_tun' not in parameters.host_tags:
            return

        self.ip = network_info.get_ip_addresses(parameters.fqdn)
        if not self.ip:
            raise ValueError('cannot resolve fqdn')

        self.has_ip4_link = bool(
            self.ip['ip4']) and 'ipip6' not in parameters.host_tags

        i = 200
        self.network_info = network_info
        self.generate_outbound_mtu = 'ipvs_tun_no_mtu' not in parameters.host_tags
        self.generate_communal_routing = 'ipvs_tun_communal' in parameters.host_tags
        self.ip4tun = 'tunl0'
        self.ip6tun = 'ip6tnl0'
        self.input_table = 300
        self.output_table = 200

        if self.has_ip4_link:
            default_iface = filter(
                lambda i: i.name == parameters.default_iface and i.address == self.ip['ip4'], interfaces_group)[0]
            ip4_vips = filter(lambda vip: network_info.is_ip(
                vip) == 4, network_info.tunnel_vips(self.ip['ip4']))

            if ip4_vips:
                tunl_iface = self.tunnel(default_iface, parameters, ip4=True)
                interfaces_group.add_interface(tunl_iface)

            for vip in ip4_vips:
                loopback = self.vip_loopback(i, vip, like_ip4=True)
                i += 1
                interfaces_group.add_interface(loopback)

        if self.ip['ip6']:
            default_iface = filter(
                lambda i: i.name == parameters.default_iface and i.address == self.ip['ip6'], interfaces_group)[0]
            ip6_vips = network_info.tunnel_vips(self.ip['ip6'])
            need4to6 = {}
            if not self.has_ip4_link:
                need4to6 = self.need4to6(network_info, ip6_vips)

            if ip6_vips:
                tunl_iface = self.tunnel(default_iface, parameters, ip4=False)
                interfaces_group.add_interface(tunl_iface)

            for vip in ip6_vips:
                loopback = self.vip_loopback(i, vip, need4to6=need4to6.get(
                    vip, False), default_iface=default_iface)
                i += 1
                interfaces_group.add_interface(loopback)

            if need4to6:
                self.generate4to6(interfaces_group,
                                  self.ip['ip6'], network_info, parameters)

        if i > 200:
            loopback = filter(lambda i: i.name == 'lo', interfaces_group)[0]
            loopback.add_postup(
                'sysctl "net.ipv4.conf.all.rp_filter=0" || true')
            loopback.add_postup(
                'sysctl "net.ipv4.conf.default.rp_filter=0" || true')

    def tunnel(self, default_iface, parameters, ip4=True):
        name = self.ip4tun if ip4 else self.ip6tun
        modprobe = 'ipip' if ip4 else 'ip6_tunnel'

        tunl_iface = TunnelInterface(self.network_info, modprobe=modprobe)
        tunl_iface.name = name
        tunl_iface.mtu = parameters.mtu
        tunl_iface.add_postup(
            'sysctl "net.ipv4.conf.%s.rp_filter=0" || true' % name)
        if self.generate_communal_routing:
            tunl_iface.add_postup(
                'sysctl "net.ipv4.ip_nonlocal_bind=1" || true')
            tunl_iface.add_postup(Rule().add().ip4().iif(
                name).lookup(self.input_table))
            tunl_iface.add_postdown(Rule().delete().ip4().iif(
                name).lookup(self.input_table))
            if not ip4:
                tunl_iface.add_postup(Rule().add().ip6().iif(
                    name).lookup(self.input_table))
                tunl_iface.add_postdown(Rule().delete().ip6().iif(
                    name).lookup(self.input_table))
        if self.generate_outbound_mtu:
            action = Route().add().ip4().default().dev(default_iface.name)\
                .via(default_iface.gateway).mtu(tunl_iface.default_mtu).table(self.output_table)
            if not ip4:
                action = action.ip6()
            tunl_iface.add_postup(action)

            action = Route().delete().ip4().default().dev(default_iface.name)\
                .via(default_iface.gateway).mtu(tunl_iface.default_mtu).table(self.output_table)
            if not ip4:
                action = action.ip6()
            tunl_iface.add_predown(action)
        return tunl_iface

    def vip_loopback(self, i, vip, like_ip4=False, need4to6=False, default_iface=None):
        loopback = LoopbackInterface(self.network_info)
        loopback.name = 'lo:%d' % i
        loopback.itype = 'manual'
        loopback.add_up(Address().add().address(
            vip).dev('lo').label(loopback.name))
        loopback.add_down(Address().delete().address(
            vip).dev('lo').label(loopback.name))

        if self.generate_communal_routing:
            if like_ip4 or need4to6:
                loopback.add_postup(Route().delete().ip4().route(
                    'local ' + vip).table('local'))
                loopback.add_postup(Route().add().ip4().route(
                    'local ' + vip).dev('lo').table(self.input_table))
                loopback.add_postdown(Route().delete().ip4().route(
                    'local ' + vip).dev('lo').table(self.input_table))
            else:
                loopback.add_postup(Route().delete().ip6().route(
                    'local ' + vip).table('local'))
                loopback.add_postup(Route().add().ip6().route(
                    'local ' + vip).dev('lo').table(self.input_table))

                # select the proper src address to a loopback dst, see CONDUCTOR-1377
                if self.network_info.is_ip(vip) == 4 and self.has_ip4_link:
                    loopback.add_postup(
                        Route()
                        .add()
                        .ip4()
                        .route(vip)
                        .dev(default_iface.name)
                        .via(default_iface.gateway)
                        .src(self.ip['ip4'])
                        .metric(1)
                        .mtu(default_iface.encap_mtu)
                    )
                    loopback.add_postdown(
                        Route()
                        .delete()
                        .ip4()
                        .route(vip)
                        .dev(default_iface.name)
                        .via(default_iface.gateway)
                        .src(self.ip['ip4'])
                    )
                else:
                    loopback.add_postup(
                        Route()
                        .add()
                        .ip6()
                        .route(vip)
                        .dev(default_iface.name)
                        .via(default_iface.gateway)
                        .src(self.ip['ip6'])
                        .metric(1)
                        .mtu(default_iface.encap_mtu)
                    )
                    loopback.add_postdown(
                        Route()
                        .delete()
                        .ip6()
                        .route(vip)
                        .dev(default_iface.name)
                        .via(default_iface.gateway)
                        .src(self.ip['ip6'])
                    )

                loopback.add_postdown(Route().delete().ip6().route(
                    'local ' + vip).dev('lo').table(self.input_table))

        if like_ip4 and self.generate_outbound_mtu:
            loopback.add_postup(Rule().add().ip4().from_addr(
                vip).lookup(self.output_table))
            loopback.add_predown(Rule().delete().ip4().from_addr(
                vip).lookup(self.output_table))

        if need4to6:
            loopback.add_postup(Rule().add().ip4().from_addr(vip).lookup(666))
            loopback.add_predown(
                Rule().delete().ip4().from_addr(vip).lookup(666))
        elif self.generate_outbound_mtu and not like_ip4 and not (self.network_info.is_ip(vip) == 4 and self.has_ip4_link):
            loopback.add_postup(Rule().add().ip6().from_addr(
                vip).lookup(self.output_table))
            loopback.add_predown(Rule().delete().ip6().from_addr(
                vip).lookup(self.output_table))
        return loopback

    @staticmethod
    def need4to6(network_info, ip6_vips):
        ip4in6vips = filter(lambda a: network_info.is_ip(a) == 4, ip6_vips)
        result = {}
        for ip4vip in ip4in6vips:
            result[ip4vip] = True
        return result

    def generate4to6(self, interfaces, ipv6, network_info, parameters):
        iface = Interface(network_info)
        iface.do_default = False
        iface.do_ethtool = False
        iface.name = '4to6tun0'
        iface.itype = 'manual'
        iface.add_preup('ip -6 tun change %s mode any || true' % self.ip6tun)
        if 'cdn_decap' in parameters.host_tags:
            iface.add_preup(
                'ip -6 tunnel add 4to6tun0 mode ipip6 remote 2a02:6b8:0:3400::aaab local %s' % ipv6)
        else:
            iface.add_preup(
                'ip -6 tunnel add 4to6tun0 mode ipip6 remote 2a02:6b8:0:3400::aaaa local %s' % ipv6)
        iface.add_up('ifconfig 4to6tun0 up')
        iface.add_postup(Route().add().default().dev(
            '4to6tun0').table(666).mtu(1450).advmss(1410))
        iface.add_predown(Route().delete().default().dev(
            '4to6tun0').table(666).mtu(1450).advmss(1410))
        iface.add_predown('ip -6 tunnel del 4to6tun0')
        interfaces.add_interface(iface)

    def generate_ip4tunnel(self, parameters, iface_utils, network_info, interfaces_group):
        ip = network_info.get_ip_addresses(parameters.fqdn)
        if not ip['ip6']:
            return
        ip6 = ip['ip6']
        ip4tunnel = network_info.ip4_tunnel(ip6)
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
        iface.add_preup(
            'ip -6 tunnel add ip6tun0 mode ipip6 remote 2a02:6b8:b010:a0ff::1 local %s' % ip6)
        iface.add_postdown(
            'ip -6 tunnel del ip6tun0 mode ipip6 remote 2a02:6b8:b010:a0ff::1 local %s' % ip6)
        iface.add_postup('ip address add %s/32 dev ip6tun0' % ip4tunnel)
        iface.add_postdown('ip address del %s/32 dev ip6tun0' % ip4tunnel)
        iface.add_postup('ip route add 0/0 dev ip6tun0 mtu 1450')
        iface.add_postdown('ip route del 0/0 dev ip6tun0 mtu 1450')
        interfaces_group.add_interface(iface)
