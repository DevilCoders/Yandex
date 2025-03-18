#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import gencfg
from core.argparse.parser import ArgumentParserExt
import core.argparse.types as argparse_types
from core.db import CURDB


def get_parser():
    parser = ArgumentParserExt(description="Move machines to reserved")

    parser.add_argument("-s", "--hosts", type=argparse_types.grouphosts, required=True,
                        help="Obligatory. List of hosts or groups to move to reserved")
    parser.add_argument("-f", "--filter", type=argparse_types.pythonlambda, default=lambda x: True,
                        help="Optinal. Filter on hosts to process")
    parser.add_argument("-v", "--verbose", action="count", default=0,
                        help="Optional. Increase verbose level")

    return parser


def main(options):
    filtered_hosts = filter(lambda x: options.filter(x), options.hosts)

    if options.verbose > 0:
        print "Move to reserved: %s" % " ".join(map(lambda x: x.name, filtered_hosts))

    for host in filtered_hosts:
        CURDB.groups.move_host(host, CURDB.groups.get_group('%s_RESERVED' % host.location.upper()))

    CURDB.update()


if __name__ == '__main__':
    options = get_parser().parse_cmd()
    main(options)
