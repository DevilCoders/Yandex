#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

from collections import defaultdict

import gencfg
from core.argparse.parser import ArgumentParserExt
import core.argparse.types as argparse_types

RESOURCE_COEFFS = [
    ('power', lambda x: x.power, 0.35 / 1293.),  # 2xE5-2650v2 is 1293. power units and 35% of etalon machine cost
    ('memory', lambda x: x.botmem, 0.4 / 248.),  # 256Gb memory is 40% of etalon machine cost
    ('hdd', lambda x: x.botdisk, 0.025 / 3445),  # 3445Gb of disk is 2.5% of etalon machine cost
    ('ssd', lambda x: x.botssd, 0.225 / 1648),  # 1648Gb of ssd is 22.5% of etalon machine cost
]


def get_parser():
    parser = ArgumentParserExt(
        description="Calculate, how much etalon machine we save by returning specified machines into cloud")
    parser.add_argument("-s", "--hosts", type=argparse_types.grouphosts, default=None,
                        help="Obligatory. Comma-separated list of hosts or groups")
    parser.add_argument("--ignore-missing-hosts", action="store_true", default=False,
                        help="Optional. Ignore hosts, which are not in our base")
    parser.add_argument("-r", "--resource-dict", type=argparse_types.jsondict, default=None,
                        help="Optional. Get saved memory/disk/ssd/cpu power as dict with keys power,memory(GB),hdd(GB),ssd(GB) (e. g. '{\"memory\" : 84000, \"hdd\" : 12345, \"ssd\" : 333, \"power\" : 444 }')")
    parser.add_argument("-v", "--verbose", action="count", dest="verbose_level", default=0,
                        help="Optional. Verbose mode. Multiple -v options increase the verbosity. The maximum is 1.")

    return parser


def normalize(options):
    if options.hosts is None and options.resource_dict is None:
        raise Exception("Options <--hosts> and <--resource-dict> are mutually exclusive")

    if options.resource_dict is not None:
        input_keys = options.resource_dict.keys()
        avail_keys = map(lambda (x, y, z): x, RESOURCE_COEFFS)

        notfound_keys = set(input_keys) - set(avail_keys)
        if len(notfound_keys) > 0:
            raise Exception("Resource types <%s> not found" % (",".join(notfound_keys)))


def main(options):
    gain_dict = defaultdict(float)

    if options.hosts is not None:
        for host in options.hosts:
            for resource_name, resource_func, gain_per_unit in RESOURCE_COEFFS:
                gain_dict[resource_name] += resource_func(host) * gain_per_unit
    if options.resource_dict is not None:
        for resource_name, resource_func, gain_per_unit in RESOURCE_COEFFS:
            gain_dict[resource_name] += options.resource_dict.get(resource_name, 0) * gain_per_unit

    return {
        'gain': gain_dict,
    }


def print_result(result, options):
    total_gain = sum(result['gain'].values())

    suffixes = []
    if options.hosts is not None:
        suffixes.append("from %d hosts" % (len(options.hosts)))
    if options.resource_dict is not None:
        suffixes.append("from specified resources savings")
    suffix = " and ".join(suffixes)

    print "Total gain %s: %.2f" % (suffix, total_gain)

    if options.verbose_level >= 1:
        for k in sorted(result['gain'].keys()):
            print "    Gained by resource %s: %.2f" % (k, result['gain'][k])


if __name__ == '__main__':
    options = get_parser().parse_cmd()

    normalize(options)

    result = main(options)

    print_result(result, options)
