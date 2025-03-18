#!/skynet/python/bin/python
import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

from collections import defaultdict

import gencfg
from core.db import CURDB
from core.argparse.parser import ArgumentParserExt
import core.argparse.types as argparse_types


def get_parser():
    parser = ArgumentParserExt(description="Calculate number of replicas in intlookup")
    parser.add_argument("-i", "--intlookups", type=argparse_types.intlookups, required=True,
                        help="Obligatory. Path to intlookup")
    parser.add_argument("-f", "--filter", type=argparse_types.pythonlambda, dest="flt", default=lambda x: True,
                        help="Optional. Filter on instances")
    parser.add_argument("-x", "--exclude", type=argparse_types.argparse_hosts_or_instances, default=None,
                        help="Optional. Hosts to exclude")
    parser.add_argument("-s", "--split-host-instances", action="store_true", default=False,
                        help="Optional. Split host instances (count 2 instance on single host as 2 repilcas)")
    parser.add_argument("-w", "--split-by-switch", action="store_true", default=False,
                        help="optional. Splty by switch")
    parser.add_argument("-p", "--show-power", action="store_true", default=False,
                        help="Optional. Show shard power instead of replicas count")
    parser.add_argument("-j", "--join-intlookups", action="store_true", default=False,
                        help="Optional. Join input inlookups as single intlookups")
    parser.add_argument("-m", "--min-replicas", type=int, default=None,
                        help="Optional. Fail if min replicas less than specified value")
    parser.add_argument('-e', '--exact-replicas', type=int, default=None,
                        help='Optional. Fail if number of replicas is not equal to specified value')

    return parser


def normalize(options):
    if options.join_intlookups:
        multishards = options.intlookups[0].get_multishards()
        for intlookup in options.intlookups[1:]:
            other_multishards = intlookup.get_multishards()
            assert len(multishards) == len(other_multishards), "Intlookups <%s> and <%s> have different shard count: %s and %s" % (
                                                                options.intlookups[0].file_name, intlookup.file_name, len(multishards),
                                                                len(other_multishards))
            for i in xrange(len(multishards)):
                multishards[i].brigades.extend(other_multishards[i].brigades)
        options.intlookups = [options.intlookups[0]]

    if options.exclude:
        exclude = sum(map(lambda x: CURDB.groups.get_host_instances(x), options.exclude.hosts), [])
        for hostname in options.exclude.instances:
            host_instances = CURDB.groups.get_host_instances(hostname)
            exclude.extend(filter(lambda x: x.port in options.exclude.instances[hostname], host_instances))

        options.exclude = list(set(exclude))

    return options


def main(options):
    intlookups_info = []

    return_status = 0
    for intlookup in options.intlookups:
        by_count = defaultdict(float)

        min_replicas, max_replicas, total_replicas = 10000000, 0, 0
        for shard_id in range(intlookup.get_shards_count()):
            instances = intlookup.get_base_instances_for_shard(shard_id)
            if options.flt:
                instances = filter(options.flt, instances)
            if options.exclude:
                instances = filter(lambda x: x not in options.exclude, instances)

            if options.split_host_instances:
                n = len(set(instances))
            elif options.show_power:
                n = sum(map(lambda x: x.power, instances))
            elif options.split_by_switch:
                n = len(set(map(lambda x: x.host.switch, instances)))
            else:
                n = len(set(map(lambda x: x.host.name, instances)))

            by_count[n] += 1
            min_replicas = min(n, min_replicas)
            max_replicas = max(n, max_replicas)
            total_replicas += n

            if options.min_replicas and options.min_replicas > min_replicas:
                return_status = 1

            if (options.exact_replicas is not None) and (options.exact_replicas != min_replicas or options.exact_replicas != max_replicas):
                return_status = 1

        intlookups_info.append({
            "file_name": intlookup.file_name,
            "min_replicas": min_replicas,
            "avg_replicas": total_replicas / float(intlookup.get_shards_count()),
            "max_replicas": max_replicas,
            "shards_by_replicas": by_count,
        })

    return {"intlookups_info": intlookups_info, "return_status": return_status}


def print_result(result):
    for elem in result:
        print "%s: %.2f/%.2f/%.2f (%s)" % (
        elem["file_name"], elem["min_replicas"], elem["avg_replicas"], elem["max_replicas"],
        ', '.join(
            ['%.2f: %d' % (key, elem["shards_by_replicas"][key]) for key in sorted(elem["shards_by_replicas"].keys())]))


def jsmain(d):
    options = get_parser().parse_json(d)
    normalize(options)
    return main(options)


if __name__ == '__main__':
    options = get_parser().parse_cmd()

    normalize(options)

    result = main(options)

    print_result(result["intlookups_info"])

    sys.exit(result["return_status"])
