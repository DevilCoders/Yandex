from ..route import Route
from ..rule import Rule
from ..iptables import Iptables


class PlainIfaceGenerator(object):
    def generate(self, parameters, iface_utils, network_info, interfaces):
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

        if 'cdn_fw' in parameters.host_tags:
            ips = network_info.get_ip_addresses(parameters.fqdn)
            if ips["ip4"] is not None:
                predown = iface_params.setdefault('predown4', [])
                postup = iface_params.setdefault('postup4', [])
                address = ips["ip4"]
                gatefw = network_info.get_fwgw_from_cidr(
                    network_info.get_networks(ips)['net4']['cidr'])
                postup.append(Route().add().ip4().default().via(gatefw).dev(
                    iface_params['name']).src(address).mtu(parameters.mtu).table(1000))
                postup.append(Rule().add().ip4().fwmark(
                    "0x2/0x2").lookup(1000))
                postup.append(Iptables().ip4().raw("-t mangle -A OUTPUT -s " + address +
                                                   " -p tcp -m tcp --tcp-flags SYN SYN -j MARK --set-mark 0x2"))
                postup.append(Iptables().ip4().raw("-t mangle -A OUTPUT -s " + address +
                                                   " -p tcp -m tcp --tcp-flags FIN FIN -j MARK --set-mark 0x2"))
                postup.append(Iptables().ip4().raw("-t mangle -A OUTPUT -s " + address +
                                                   " -p tcp -m tcp --tcp-flags RST RST -j MARK --set-mark 0x2"))
                predown.append(Rule().delete().ip4().fwmark(
                    "0x2/0x2").lookup(1000))
                predown.append(Iptables().ip4().raw("-t mangle -F OUTPUT"))
                predown.append(Route().delete().ip4().default().via(gatefw).dev(
                    iface_params['name']).src(address).mtu(parameters.mtu).table(1000))

        if 'nojumboroute' in parameters.host_tags:
            iface_params['jumboroute'] = False

        if parameters.bridged:
            iface_params['bridge_ports'] = iface_params['name']
            iface_params['name'] = 'br0'

            if 'bonding' in parameters.host_tags and parameters.bonding:
                iface_params['bridge_ports'] = 'bond0'

        # this array might not be populated at all
        iface_params['preup'] = []
        if 'disable_dad' in parameters.host_tags:
            iface_params['preup'].append(
                'sysctl net.ipv6.conf.' + iface_params['name'] + '.accept_dad=0')

        if parameters.multiqueue:
            iface_params['preup'].append(
                'ethtool -L ' + iface_params['name'] + ' combined $(nproc) 2>/dev/null || true')

        ifaces = iface_utils.generate_plain_interface(
            parameters.fqdn, iface_params, skip_ip4=parameters.skip_ip4)

        if 'bonding' in parameters.host_tags and not parameters.bridged:
            bond = filter(lambda i: i.name == 'bond0', interfaces)[0]
            for iface in ifaces:
                pre_ups = iface.preup
                post_ups = iface.postup
                ups = iface.up
                pre_downs = iface.predown
                post_downs = iface.postdown
                jumbo = iface.jumbo_nets
                auto = iface.auto
                iface.update(bond)
                iface.preup += pre_ups
                iface.postup += post_ups
                iface.up += ups
                iface.predown += pre_downs
                iface.postdown += post_downs
                iface.jumbo_nets += jumbo
                iface.auto = auto
            interfaces.remove_interface(bond)
        interfaces.add_interfaces(ifaces)
