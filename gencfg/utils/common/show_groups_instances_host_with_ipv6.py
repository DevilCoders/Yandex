#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

from argparse import ArgumentParser
from collections import defaultdict

import gencfg
from core.db import CURDB
import core.argparse.types as argparse_types
import gaux.aux_resolver
import gaux.aux_hbf


def parse_cmd():
    parser = ArgumentParser(description="Show host and ipv6 for instances of certain groups")
    parser.add_argument("-g", "--groups", dest="groups", type=argparse_types.groups, required=True,
                        help="List of groups")
    parser.add_argument("-o", "--output", dest="output", type=str, required=True,
                        help="Path to the output")

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    return parser.parse_args()


def main(options):
    hosts = []
    for group in options.groups:
        for instance in group.get_kinda_busy_instances():
            host = gaux.aux_hbf.generate_mtn_hostname(instance, group, '')
            hosts.append(host)
        for host in group.get_hosts():
            hosts.append(host.name)

    return ["{}={}".format(host, ip) for host, ip in gaux.aux_resolver.resolve_hosts(hosts, [6], fail_unresolved=False).iteritems()]


if __name__ == '__main__':
    options = parse_cmd()

    result = main(options)
    with open(options.output, 'w') as f:
        f.write('\n'.join(result))
