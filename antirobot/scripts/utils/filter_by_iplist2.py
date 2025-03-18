#!/usr/bin/env python

import sys
import ipaddr
from optparse import OptionParser


class IpList:
    def __init__(self, file_name):
        self.nets = self._load_list(file_name)

    @staticmethod
    def _load_list(file_name):
        result = []

        for line in open(file_name):
            line = line.strip()
            if not line or line.startswith('#'):
                continue

            line = line.split('#', 1)[0].strip()

            try:
                net = ipaddr.IPNetwork(line)
                result.append(net)
            except Exception, ex:
                print >>sys.stderr, ex

        return result

    def Contains(self, ip_addr):
        try:
            ip = ipaddr.IPAddress(ip_addr)
        except:
            print >>sys.stderr, "Could not parse %s as valid ip address" % ip_addr
            return False

        for net in self.nets:
            if net.Contains(ip):
                return True

        return False


def main():
    usage = """
    %prog [options] ipnet_list_file

The program will read ip from stdin and filter it by given ipnet list.

ip networks in the list are look like:
  192.168.1.0/24
  1022:12:30:3::/64
"""
    parser = OptionParser(usage)
    parser.add_option("-v", dest="exclude", help="exclude mode", default=False, action="store_true")

    (options, args) = parser.parse_args()
    if len(args) == 0:
        parser.print_help()
        return

    ipList = IpList(args[0])

    for line in sys.stdin:
        ip_str = line.strip()

        ipInList = ipList.Contains(ip_str)

        if ipInList ^ options.exclude:
            print line,


if __name__ == "__main__":
    main();
