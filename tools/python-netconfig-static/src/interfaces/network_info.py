
import interfaces
import json
import socket

from ipaddr import IPAddress, IPNetwork
from copy import copy
from .resolver import resolve4_first, resolve4_all, resolve6_first, resolve6_all


PROHIBITED_IP = ['93.158.129.171', '213.180.204.242', '213.180.205.115']
PROHIBITED_IP6 = []


class NetworkInfo(object):
    def __init__(self, conductor, dns_data):
        if not conductor:
            raise ValueError('pass conductor to construct NetworkInfo')
        self.__conductor = conductor
        if dns_data:
            import dns_mock
            if isinstance(dns_data, str):
                dns_data = json.loads(open(dns_data).read())
            self.dns_data = dns_mock.prepare(dns_data)
        else:
            self.dns_data = None

    def is_ip(self, s):
        try:
            ip = IPNetwork(s)
        except:
            return None
        return ip.version

    def get_ip_addresses(self, fqdn):
        if interfaces.DEBUG:
            print "Resolving %s locally" % fqdn
        ip4 = self.get_host_ip(fqdn)
        ip6 = self.get_host_ip6(fqdn)
        if ip4 in PROHIBITED_IP:
            ip4 = None
        if ip6 in PROHIBITED_IP6:
            ip6 = None
        if ip4 is None and ip6 is None:
            if interfaces.DEBUG:
                print "Could not resolve"
                print "------------------------"
            return None
        if interfaces.DEBUG:
            print "IP4: %s, IP6: %s" % (ip4, ip6)
            print "------------------------"
        return dict(ip4=ip4, ip6=ip6)

    def get_all_ip_addresses(self, fqdn):
        if interfaces.DEBUG:
            print "Resolving %s locally (all ips)" % fqdn
        ips = [(4, ip)
               for ip in self.get_all_ips4(fqdn) if ip not in PROHIBITED_IP]
        ips += [(6, ip)
                for ip in self.get_all_ips6(fqdn) if ip not in PROHIBITED_IP6]
        if not ips:
            if interfaces.DEBUG:
                print "Could not resolve"
                print "------------------------"
            return None
        if interfaces.DEBUG:
            print repr(ips)
            print "------------------------"
        return ips

    def get_networks(self, opts):
        additional_vlans = {
            622: [577],
            577: [622],
            777: [722, 736],
            722: [777, 736],
            736: [722, 777],
        }

        search_vlans = [
            150, 503, 509,
            522, 531, 550,
            565, 567, 585,
            589, 593, 594,
            595, 599, 603,
            604, 613, 637,
            653, 664, 665,
            667, 690, 695,
            696, 698,
        ]

        for sv in search_vlans:
            m = copy(search_vlans)
            m.remove(sv)
            additional_vlans[sv] = m

        net4 = None
        net6 = None
        if opts['ip4']:
            net4 = self.__conductor.ip_info(opts['ip4'])
        if opts['ip6']:
            net6 = self.__conductor.ip_info(opts['ip6'])
        if interfaces.DEBUG:
            print "Getting network info for %s" % repr(opts)
            print "NET4: %s" % repr(net4)
            print "NET6: %s" % repr(net6)
            print "------------------------"
        return {'net4': net4, 'net6': net6}

    def cdn(self, ip):
        return self.__conductor.ip_info(ip)['cdn_vips']

    def aliases(self, ip):
        return self.__conductor.ip_info(ip)['aliases']

    def slb(self, ip):
        return self.__conductor.ip_info(ip)['virtual_services']

    def tunnel_vips(self, ip):
        return self.__conductor.ip_info(ip)['tunnel_vips']

    def ip4_tunnel(self, ip):
        return self.__conductor.ip_info(ip).get('ip4tunnel', None)

    def get_host_ip(self, fqdn):
        if self.dns_data:
            data = self.dns_data[fqdn][4]
            return data and data[0] or None
        return resolve4_first(fqdn)

    def get_all_ips4(self, fqdn):
        if self.dns_data:
            return self.dns_data[fqdn][4]
        return resolve4_all(fqdn)

    def get_host_ip6(self, fqdn):
        if self.dns_data:
            return self.dns_data[fqdn][6][0]
        return resolve6_first(fqdn)

    def get_all_ips6(self, fqdn):
        if self.dns_data:
            return self.dns_data[fqdn][6]
        return resolve6_all(fqdn)

    def get_host_fqdn(self, ip):
        try:
            return socket.gethostbyaddr(ip)[0]
        except socket.error:
            return None

    def get_host_fqdn6(self, ip):
        return self.get_host_fqdn(ip)

    def get_net_from_cidr(self, cidr):
        net = {}
        network = IPNetwork(cidr)
        net['ip'] = str(network.ip)
        if network.version == 4:
            net['mask'] = str(network.netmask)
        else:
            net['mask'] = network.prefixlen
        return net

    def get_gateway_from_cidr(self, cidr):
        network = IPNetwork(cidr)
        return str(IPAddress(int(network.broadcast) - 1))

    def get_fwgw_from_cidr(self, cidr):
        network = IPNetwork(cidr)
        return str(IPAddress(int(network.broadcast) - 2))

    def get_net_from_ip(self, address, mask):
        network = IPNetwork("%s/%s" % (address, mask))
        return str(network.network)

    def get_ipv6_gw_from_cidr(self, cidr):
        network = IPNetwork(cidr)
        return str(IPAddress(int(network.network) + 1))
