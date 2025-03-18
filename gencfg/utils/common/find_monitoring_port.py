#!/skynet/python/bin/python

"""Script to calculate monitoring port for group"""

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import gencfg
from core.db import CURDB
from core.argparse.parser import ArgumentParserExt
import core.argparse.types
from utils.common.find_most_unused_port import FORBIDDEN_PORTS


def get_parser():
    parser = ArgumentParserExt(description="Find monitoring port for group")
    parser.add_argument("-g", "--group", type=core.argparse.types.group, required=True,
                        help="Group to find free port for")

    return parser


def main(options):
    used_ports = set()
    for host in options.group.getHosts():
        for instance in CURDB.groups.get_host_instances(host):
            group = CURDB.groups.get_group(instance.type)
            if group.card.legacy.funcs.instancePort.startswith('new'):
                N = 8
            else:
                N = 1

            for i in xrange(N):
                used_ports.add(instance.port + i)

            if group.card.properties.monitoring_juggler_port is not None:
                used_ports.add(group.card.properties.monitoring_juggler_port)
            if group.card.properties.monitoring_golovan_port is not None:
                used_ports.add(group.card.properties.monitoring_golovan_port)

    used_ports |= set(FORBIDDEN_PORTS.keys())

    if options.group.card.properties.monitoring_juggler_port is not None:
        used_ports.add(options.group.card.properties.monitoring_juggler_port)
    if options.group.card.properties.monitoring_golovan_port is not None:
        used_ports.add(options.group.card.properties.monitoring_golovan_port)

    for candidate in xrange(31000, 21000, -1):
        if candidate not in used_ports:
            return candidate

    raise Exception('Could not allocate monitoring port for <{}>'.format(group.card.name))


def jsmain(d):
    options = get_parser().parse_json(d)

    return main(options)


if __name__ == '__main__':
    options = get_parser().parse_cmd()

    port = main(options)

    print 'Monitoring port for {} is {}'.format(options.group.card.name, port)
