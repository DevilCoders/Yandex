__author__ = 'yottatsa'

import socket
import struct
import re

import salt.utils.network


def interface():
    AF_INET = socket.AF_INET
    AF_INET6 = socket.AF_INET6

    def gateways():
        """Read the default gateway directly from /proc."""
        retval = {'default': {}}

        try:
            with open("/proc/net/route") as fh:
                for line in fh:
                    fields = line.strip().split()
                    if fields[1] != '00000000' or not int(fields[3], 16) & 2:
                        continue

                    retval['default'][AF_INET] = [socket.inet_ntop(AF_INET, struct.pack("<L", int(fields[2], 16))),
                                              fields[0]]
        except:
            pass

        try:
            defroute = ['00000000000000000000000000000000', '00', '00000000000000000000000000000000', '00']
            with open("/proc/net/ipv6_route") as fh:
                for line in fh:
                    fields = line.strip().split()
                    if fields[0:4] != defroute or not int(fields[8], 16) & 2:
                        continue

                    retval['default'][AF_INET6] = [
                        socket.inet_ntop(AF_INET6, socket.inet_pton(AF_INET6, re.sub('(....)', '\\1:', fields[4], 7))),
                        fields[9]]
        except:
            pass

        return retval

    def ifaddresses(name):
        ifaces = salt.utils.network.interfaces()  # pylint: disable=E1101
        retval = {}
        if name not in ifaces and 'inet' not in ifaces[name]:
            return
        if 'inet' in ifaces[name]:
            for ipv4 in ifaces[name]['inet']:
                ipv4['addr'] = ipv4['address']
                del ipv4['address']
                del ipv4['label']
            retval[AF_INET] = ifaces[name]['inet']
        if 'inet6' in ifaces[name]:
            for ipv6 in ifaces[name]['inet6']:
                ipv6['addr'] = ipv6['address']
                del ipv6['address']
            retval[AF_INET6] = ifaces[name]['inet6']
        return retval

    interface = {}
    gws = gateways()
    if gws and 'default' in gws:
        default = gws['default']

        if AF_INET in default:
            interface['v4_route'] = default[AF_INET][0]
            interface['v4_addr'] = ifaddresses(default[AF_INET][1])[AF_INET][0]
            interface['mode'] = 'v4'
            interface.update(interface['v4_addr'])

        if AF_INET6 in default:
            interface['v6_route'] = default[AF_INET6][0]
            interface['v6_addr'] = [
                iface
                for iface in ifaddresses(default[AF_INET6][1])[AF_INET6]
                if not iface['addr'].startswith('f')]
            if interface['v6_addr']:
                interface.update(interface['v6_addr'][0])
                interface['mode'] = 'v6'
            else:
                del default[AF_INET6]

        if AF_INET in default and AF_INET6 in default:
            interface['mode'] = 'dualstack'

    return {
        'interface': interface
    }


if __name__ == '__main__':
    print(interface())
