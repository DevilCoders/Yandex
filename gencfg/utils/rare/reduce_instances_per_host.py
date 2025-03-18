#!/skynet/python/bin/python

"""
    Suppose we have group with <N> instances per host distributed among intlookup shards. And we know, that <N> instances per host is too much.
    In this script we thin out intlooup, removing extra instances and making result intlookup as unifrom (by repilcas count and power) as possible
"""

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

from collections import defaultdict

import gencfg
from core.db import CURDB
from core.argparse.parser import ArgumentParserExt
import core.argparse.types as argparse_types
from gaux.aux_colortext import red_text


def get_parser():
    parser = ArgumentParserExt(description="Reduce instances per host in intlookup")
    parser.add_argument("-i", "--intlookup", type=argparse_types.intlookup, required=True,
                        help="Obligatory. Intlookup name")
    parser.add_argument("-n", "--max-instances", type=int, required=True,
                        help="Obligatory. Maximal instances per host in result intlookup")
    parser.add_argument("-y", "--apply", action="store_true", default=False,
                        help="Optional. Apply changes")

    return parser


def normalize(options):
    if options.intlookup.hosts_per_group > 1:
        raise Exception("Intlookup <%s> has <%d> hosts per group, which is not yet supported" % (options.intlookup.file_name, options.intlookup.hosts_per_group))


def main(options):
    options.intlookup.mark_as_modified()

    host_instance_count = defaultdict(int)
    for instance in options.intlookup.get_base_instances():
        host_instance_count[instance.host] += 1

    multishards = options.intlookup.get_multishards()

    for host in host_instance_count:
        for i in range(host_instance_count[host] - options.max_instances):
            best_multishard = None
            for multishard in multishards:
                if host not in map(lambda x: x.basesearchers[0][0].host, multishard.brigades):
                    continue
                if best_multishard is None or len(best_multishard.brigades) < len(multishard.brigades):
                    best_multishard = multishard

            assert best_multishard is not None, "OOPS, could not happen"

            for index, brigade in enumerate(best_multishard.brigades):
                if brigade.basesearchers[0][0].host == host:
                    best_multishard.brigades.pop(index)
                    break

    print "Intlookup <%s> replicas count:" % options.intlookup.file_name
    for shardid, multishard in enumerate(multishards):
        print "    Shard %s: %d replicas" % (shardid, len(multishard.brigades))

    if options.apply:
        CURDB.intlookups.update(smart=True)
    else:
        print red_text("Not updated!!! Add option -y to update.")

    return None

if __name__ == '__main__':
    options = get_parser().parse_cmd()

    normalize(options)

    result = main(options)
