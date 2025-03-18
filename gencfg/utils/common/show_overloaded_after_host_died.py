#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

from collections import defaultdict

import gencfg
from core.argparse.parser import ArgumentParserExt
from core.db import CURDB
import core.argparse.types as argparse_types


class OverloadedByHost(object):
    def __init__(self, host):
        self.host = host
        self.overloaded = defaultdict(float)

    def is_limit_reached(self, limitp):
        return len(filter(lambda (x, y): y > x.power * limitp, self.overloaded.iteritems())) > 0

    def show(self, limitp, verbose_level):
        overloaded = filter(lambda (x, y): y > x.power * limitp, self.overloaded.iteritems())
        if verbose_level <= 1 and len(overloaded) == 0:
            return None

        if verbose_level >= 1:
            overloaded = self.overloaded.items()

        overloaded.sort(cmp=lambda (x1, y1), (x2, y2): cmp(x1.name, x2.name))

        if len(overloaded) == 0:
            return None

        result = "Host %s:\n" % self.host.name
        result += "    %s" % (" ".join(map(lambda (x, y): "%s(%.1f%%)" % (x.name, y / x.power * 100.), overloaded)))
        return result


def get_parser():
    parser = ArgumentParserExt(description="Show most overloaded hosts in case some other host died")
    parser.add_argument("-c", "--sasconfig", type=argparse_types.sasconfig, required=False,
                        help="Optional. File with sas optimizer config")
    parser.add_argument("-i", "--intlookups", type=argparse_types.intlookups, default=[],
                        help="Optional. Intlookups to process")
    parser.add_argument("-M", "--max-overload", type=float, required=True,
                        help="Obligatory. Maximal allowed overload (1.0 means cpu usage can be doubled)")
    parser.add_argument("-v", "--verbose", action="count", dest="verbose_level", default=0,
                        help="Optional. Verbose mode. Multiple -v options increase the verbosity. The maximum is 2.")
    parser.add_argument("--fail-on-error", action="store_true", default=False,
                        help="Optional. Exit with non-zero status on error")
    return parser


def normalize(options):
    if options.intlookups is None and options.sasconfig is None:
        raise Exception("You must specify at least one of --intlookups --sas-config")

    if options.sasconfig is not None:
        for elem in options.sasconfig:
            options.intlookups.append(CURDB.intlookups.get_intlookup(elem.intlookup))


def calculate_overload(host, shards_by_host, hosts_by_shard, host_shards_power):
    result = OverloadedByHost(host)

    affected_shards = shards_by_host[host]
    for affected_shard in affected_shards:
        total_power = sum(map(lambda x: host_shards_power[(x, affected_shard)], hosts_by_shard[affected_shard]))
        overload_power = host_shards_power[(host, affected_shard)]
        coeff = total_power / (total_power - overload_power) - 1.

        for other_host in hosts_by_shard[affected_shard]:
            if other_host == host:
                continue
            result.overloaded[other_host] += host_shards_power[(other_host, affected_shard)] * coeff

    return result


def print_result(result, options):
    for entry in result:
        to_print = entry.show(options.max_overload, options.verbose_level)
        if to_print is not None:
            print to_print


def main(options):
    shards_by_host = defaultdict(set)
    hosts_by_shard = defaultdict(set)
    host_shards_power = defaultdict(float)

    # init working dicts
    start_shard = 0
    for intlookup in options.intlookups:
        for shard_id in range(0, intlookup.get_shards_count()):
            for instance in intlookup.get_base_instances_for_shard(shard_id):
                real_shard_id = start_shard + shard_id

                shards_by_host[instance.host].add(real_shard_id)
                hosts_by_shard[real_shard_id].add(instance.host)
                host_shards_power[(instance.host, real_shard_id)] += instance.power

    result = []
    for host in shards_by_host:
        result.append(calculate_overload(host, shards_by_host, hosts_by_shard, host_shards_power))

    return result


if __name__ == '__main__':
    options = get_parser().parse_cmd()
    normalize(options)

    result = main(options)

    print_result(result, options)

    if options.fail_on_error:
        if len(filter(lambda x: x.is_limit_reached(options.max_overload), result)) > 0:
            sys.exit(1)
        else:
            sys.exit(0)
    else:
        sys.exit(0)
