#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

from argparse import ArgumentParser
from collections import defaultdict

import gencfg
import core.argparse.types as argparse_types


def parse_cmd():
    parser = ArgumentParser(description="Find machines for update")
    parser.add_argument("-g", "--groups", type=argparse_types.groups, required=True,
                        help="Obligatory. List for groups to find machines")
    parser.add_argument("-t", "--total", type=int, required=True,
                        help="Obligatory. Number of hosts")
    parser.add_argument("-f", "--filter", type=argparse_types.pythonlambda, default=lambda x: True,
                        help="Optional. Filter on hosts")

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    options = parser.parse_args()

    return options


def main(options):
    hosts = list(set(sum(map(lambda x: x.getHosts(), options.groups), [])))
    hosts = filter(options.filter, hosts)

    hosts_by_switch = defaultdict(list)
    for host in hosts:
        hosts_by_switch[host.switch].append(host)

    switch_counts = map(lambda (x, y): (x, len(y)), hosts_by_switch.iteritems())
    switch_counts.sort(cmp=lambda (x1, y1), (x2, y2): cmp(y2, y1))

    result = []
    while options.total > 0:
        switch, N = switch_counts.pop(0)

        result.extend(hosts_by_switch[switch])
        options.total -= N

    return result


if __name__ == '__main__':
    options = parse_cmd()
    result = main(options)

    print ','.join(map(lambda x: x.name, result))
