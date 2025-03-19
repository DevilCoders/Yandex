from ..cdn_loopback_interface import CdnLoopbackInterface


class CdnIfaceGenerator(object):
    def generate(self, parameters, iface_utils, network_info, interfaces):
        if parameters.skip_ip4:
            return
        ip = network_info.get_ip_addresses(parameters.fqdn)
        i = 300
        if ip and ip['ip4']:
            for ip in network_info.cdn(ip['ip4']):
                fqdn = network_info.get_host_fqdn(ip)

                iface = CdnLoopbackInterface(network_info)
                iface.name = 'lo:%d' % i
                iface.comments.append('ubic dummy: ' + (fqdn or ip))
                iface.address = ip
                iface.do_default = False
                iface.do_ethtool = False
                if 'cdn-loopback-iface-no-auto' in parameters.host_tags:
                    iface.auto = False
                interfaces.add_interface(iface)
                i += 1
