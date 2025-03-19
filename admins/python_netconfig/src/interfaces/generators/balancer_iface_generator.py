from ..route import Route
from ..rule import Rule
from ..interface import Interface
from ..address import Address


def _compare_ip4(ip1, ip2):
    ip1 = map(lambda s: int(s), ip1.split('/')[0].split('.'))
    ip2 = map(lambda s: int(s), ip2.split('/')[0].split('.'))
    r = 0
    for i in range(len(ip1)):
        r = ip1[i] - ip2[i]
        if r != 0:
            break
    return r


class BalancerIfaceGenerator(object):
    def generate(self, parameters, iface_utils, network_info, interfaces_group):
        if not parameters.do10:
            return

        ip = network_info.get_ip_addresses(parameters.fqdn)
        if not ip:
            raise ValueError('cannot resolve fqdn')
        if parameters.skip_ip4:
            ip['ip4'] = None

        addresses = {'ip4': [], 'ip6': []}
        if ip['ip6']:
            for adr in network_info.aliases(ip['ip6']):
                if network_info.is_ip(adr) == 4:
                    addresses['ip4'].append(adr)
                if network_info.is_ip(adr) == 6:
                    addresses['ip6'].append(adr)
        if ip['ip4']:
            addresses['ip4'] += network_info.aliases(ip['ip4'])
            addresses['ip4'] = sorted(list(set(addresses['ip4'])), cmp=_compare_ip4)

        if addresses['ip6']:
            filtered = filter(
                lambda i: i.address == ip['ip6'] and i.name == parameters.default_iface, interfaces_group.interfaces()
            )
            if filtered:
                ip6_iface = filtered[0]
                actions = self.postup_for_ip6_balancers(parameters.default_iface, network_info, addresses['ip6'])
                ip6_iface.add_postups(actions['postup'])
                ip6_iface.add_predowns(actions['predown'])
                if not parameters.no_antimartians:
                    actions = self.generate_loopback_postup_ip6(
                        iface_utils, network_info, parameters.fqdn, interfaces_group
                    )
                    ip6_iface.add_postups(actions['postup'])
                    ip6_iface.add_predowns(actions['predown'])

        if addresses['ip4']:
            if 'ipvs_internal' in parameters.host_tags:  # ADMINTOOLS-354
                mtu = 8950
            else:
                mtu = 1450
            interfaces_group.add_interfaces(
                self.generate_balancer_interfaces(addresses['ip4'], mtu, parameters.default_iface, network_info)
            )
            if not parameters.no_antimartians:
                interfaces_group.add_interfaces(
                    self.generate_loopbacks(parameters.fqdn, iface_utils, network_info, interfaces_group)
                )

    def generate_balancer_interfaces(self, aliases, mtu, default_iface, network_info):
        opts = {'do_default': False, 'vlan': None, 'netmask': '255.255.255.0'}
        result = []
        for address in aliases:
            iface = opts.copy()
            if address.find('/') > -1:
                net = network_info.get_net_from_cidr(address)
                iface['address'] = net['ip']
                iface['netmask'] = net['mask']
            else:
                iface['address'] = address
            iface_num = iface['address'].split('.')[1]
            iface['name'] = default_iface + ':' + iface_num
            iface['do_ethtool'] = False
            if address.find('/') > -1:
                gate = network_info.get_gateway_from_cidr(address)
            else:
                gate = '.'.join(address.split('.')[:3] + ['254'])
            postup = []
            predown = []

            postup.append(
                Route()
                .replace()
                .ip4()
                .default()
                .via(gate)
                .dev(default_iface)
                .src(iface['address'])
                .mtu(mtu)
                .table(iface_num)
            )
            postup.append(Rule().add().ip4().from_addr(iface['address']).lookup(iface_num).priority(iface_num))
            predown.append(Rule().delete().ip4().from_addr(iface['address']).lookup(iface_num).priority(iface_num))
            predown.append(
                Route()
                .delete()
                .ip4()
                .default()
                .via(gate)
                .dev(default_iface)
                .src(iface['address'])
                .mtu(mtu)
                .table(iface_num)
            )
            iface['postup'] = postup
            iface['predown'] = predown
            result.append(Interface.from_params(network_info, iface))
        return result

    def generate_loopbacks(self, fqdn, iface_utils, network_info, interfaces):
        loopbacks = iface_utils.get_loopbacks_by_ip_version(fqdn, 4)
        i = 100
        lo_ifaces = {}
        for l in loopbacks:
            if self.vip_generated(l['vip'], interfaces):
                continue
            iface = Interface(network_info)
            iface.name = 'lo:' + str(i)
            iface.address = l['vip']
            vip_fqdn = network_info.get_host_fqdn(l['vip'])
            if not vip_fqdn:
                vip_fqdn = l['vip']
            iface.add_comment(
                'dummy: ' + vip_fqdn + ':' + str(l['vs_port']) + ' ' + l['protocol'] + ', ' + l['comment']
            )
            iface.netmask = "255.255.255.255"
            iface.do_default = False
            iface.do_ethtool = False
            if iface.address in lo_ifaces:
                lo_ifaces[iface.address].add_comments(iface.comments)
            else:
                lo_ifaces[iface.address] = iface
            i += 1
        return sorted(lo_ifaces.values(), key=lambda i: i.name)

    def postup_for_ip6_balancers(self, main_iface, network_info, aliases):
        postup = []
        table = 500
        net = {}
        for address in aliases:
            if address.find('/') > -1:
                net = network_info.get_net_from_cidr(address)
                cidr = address
            else:
                net['ip'] = address
                net['mask'] = 64
                cidr = "%s/%s" % (net['ip'], net['mask'])

            netw = "%s/%s" % (network_info.get_net_from_ip(net['ip'], net['mask']), net['mask'])
            postup.append(Address().ip6().add().address(cidr).dev(main_iface))
            postup.append(Rule().ip6().add().from_addr(net['ip']).table(table))
            postup.append(Route().ip6().add().route(netw).dev(main_iface).table(table))
            postup.append(Route().ip6().add().default().via(network_info.get_ipv6_gw_from_cidr(cidr)).table(table))
            table += 1
        predown = []
        if postup:
            predown.append(
                "/sbin/ip -6 rule | /usr/bin/awk '{print $NF}' | /bin/grep '[0-9]' | /usr/bin/xargs -n1 /sbin/ip -6 route flush table"
            )
            predown.append(
                "/sbin/ip -6 rule | /usr/bin/awk '{print \"/sbin/ip -6 rule del from \"$(NF-2) \" lookup \"$NF}' | /bin/grep 'lookup [0-9]' | /bin/bash"
            )
        return {'postup': postup, 'predown': predown}

    def generate_loopback_postup_ip6(self, iface_utils, network_info, fqdn, interfaces):
        result = {'postup': [], 'predown': []}
        loopbacks = iface_utils.get_loopbacks_by_ip_version(fqdn, 6)
        for l in loopbacks:
            vip = l['vip']
            if self.vip_generated(vip, interfaces):
                continue
            vip_fqdn = network_info.get_host_fqdn(vip)
            comment = "dummy6: %s:%s %s, %s" % (vip_fqdn, l['vs_port'], l['protocol'], l['comment'])
            result['postup'].append(Address().ip6().add().address(vip + "/128").dev("lo").comment(comment))
            result['predown'].append(Address().ip6().delete().address(vip + "/128").dev("lo").comment(comment))
        return result

    def vip_generated(self, vip, interfaces):
        for i in interfaces:
            if i.name[:2] == 'lo':
                if [x for x in i.postup if isinstance(x, Address) and vip in x._address]:
                    return True
                if [x for x in i.up if isinstance(x, Address) and vip in x._address]:
                    return True
        return False
