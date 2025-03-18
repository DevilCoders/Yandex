#!/skynet/python/bin/python
"""
    Show some information on hosts, grouped by some criteria. We need this script for finding racks with
    smallest number of projects and some other things.
"""

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

from collections import defaultdict

import gencfg
from core.db import CURDB
from core.argparse.types import pythonlambda, grouphosts
from core.argparse.parser import ArgumentParserExt


def get_parser():
    parser = ArgumentParserExt(description="Show information on hosts, grouped by some criteria")
    parser.add_argument("-p", "--group-func", type=pythonlambda, required=True,
                        help="Obligatory. Python lambda function, which returns grouping value (e. g. 'x.rack' groups hosts by rack)")
    parser.add_argument("-f", "--filter-func", type=pythonlambda, default=lambda x: True,
                        help="Optional. Filter hosts before grouping")
    parser.add_argument("-s", "--hosts", type=grouphosts, required=False,
                        help="Optional. List of groups or hosts to process, e.g. MSK_RESERVED,ws2-200.yandex.ru,MAN_RESERVED")
    parser.add_argument("-v", "--verbose", action="count", default=0,
                        help="Optional. Increase output verbosity (maximum 2)")

    return parser


def main(options):
    hosts = options.hosts
    hosts = filter(options.filter_func, hosts)

    hosts_by_group_value = defaultdict(list)
    for host in hosts:
        hosts_by_group_value[options.group_func(host)].append(host)

    for group_value in sorted(hosts_by_group_value.keys()):
        s = "Group value <%s>:" % group_value

        value_hosts = hosts_by_group_value[group_value]
        if options.verbose >= 1:
            s += "\n    %d hosts" % (len(value_hosts))
        else:
            s += " %d hosts" % (len(value_hosts))

        groups = list(set(sum(map(lambda x: CURDB.groups.get_host_groups(x), value_hosts), [])))
        if options.verbose >= 1:
            s += "\n    %d unique groups" % (len(groups))
        else:
            s += " %d unique groups" % (len(groups))

        print s


if __name__ == '__main__':
    options = get_parser().parse_cmd()
    main(options)
