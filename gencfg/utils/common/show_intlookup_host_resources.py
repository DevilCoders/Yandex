#!/skynet/python/bin/python
"""
    Script to show distrubution of allocated to intlookup resources among hosts. Can be used, for example, to check if some hosts
    used only in this inlookup or not used at all.
"""

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

from collections import defaultdict

import gencfg
from core.argparse.parser import ArgumentParserExt
import core.argparse.types as argparse_types
from core.exceptions import UtilNormalizeException
from core.db import CURDB

REPORT_TYPES = [
    "raw"
]


def get_parser():
    parser = ArgumentParserExt(
        description="Show uniformnes of resources (such as cpu) distribution among all group hosts for specified intlookup")
    parser.add_argument("-i", "--intlookup", type=argparse_types.intlookup, required=True,
                        help="Obligatory. Intlookup to process")
    parser.add_argument("--report-type", type=str, default="raw",
                        choices=REPORT_TYPES,
                        help="Optional. How to report to console results of scrips")
    parser.add_argument("--points", type=int, default=20,
                        help="Optional. Number of points in result graph (all range of [0-100] will be splitted by <points> intervals)")
    parser.add_argument("--ignore-unused", action="store_true", default=False,
                        help="Optional. Ignore unused machines (show distribution only on machines, which are used in specified intlookup")

    return parser


def normalize(options):
    if options.points < 1:
        raise UtilNormalizeException(correct_pfname(__file__), ["points"], "Number of points must be positive number")


def main(options):
    group = CURDB.groups.get_group(options.intlookup.base_type)

    by_host_data = defaultdict(float)

    if not options.ignore_unused:
        for host in group.getHosts():
            by_host_data[host] = 0.0

    for instance in options.intlookup.get_base_instances():
        by_host_data[instance.host] += instance.power

    result = [0] * options.points

    for host, host_power_used in by_host_data.iteritems():
        index = int((host_power_used / host.power) * options.points)
        index = min(options.points - 1, index)
        result[index] += 1

    return result


def print_result(result, options):
    if options.report_type == "raw":
        total_hosts = sum(result)

        print "Intlookup %s" % options.intlookup.file_name
        for i, v in enumerate(result):
            start = i / float(options.points) * 100
            end = (i + 1) / float(options.points) * 100
            print "    %.2f%% - %.2f%% host usage: %.2f%% hosts" % (start, end, v / float(total_hosts) * 100)
    else:
        raise Exception("Unkown report type <%s>" % options.report_type)


if __name__ == '__main__':
    options = get_parser().parse_cmd()

    normalize(options)

    result = main(options)

    print_result(result, options)
