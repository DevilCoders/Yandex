#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

from argparse import ArgumentParser
from collections import defaultdict

import gencfg
from core.db import CURDB
import core.argparse.types as argparse_types


def parse_cmd():
    parser = ArgumentParser(description="Compare assigned usage with host memory")
    parser.add_argument("-s", "--hosts", type=argparse_types.grouphosts, required=True,
                        help="Obligatory. List of hosts or groups to process")
    parser.add_argument("-v", "--verbose", action="count", dest="verbose_level", default=0,
                        help="Optional. Verbose mode. Multiple -v options increase the verbosity")
    parser.add_argument("-i", "--ignore-groups", type=argparse_types.comma_list, default=[],
                        help="Optional. List of ignore groups")
    parser.add_argument("--fail-on-error", action="store_true", default=False,
                        help="Optional. Exit with non-zero status in case of error")

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    options = parser.parse_args()

    return options


def main(options):
    all_instances = sum(map(lambda x: CURDB.groups.get_host_instances(x), options.hosts), [])
    all_groups = map(lambda y: CURDB.groups.get_group(y), set(map(lambda x: x.type, all_instances)))
    all_used_instances = sum(map(lambda x: x.get_kinda_busy_instances(), all_groups), [])
    all_used_instances = filter(lambda x: x.type not in options.ignore_groups, all_used_instances)

    all_used_instances_by_host = defaultdict(list)
    for instance in all_used_instances:
        all_used_instances_by_host[instance.host].append(instance)

    failed = False

    for host in options.hosts:
        used_instances = all_used_instances_by_host[host]

        used_memory = 0
        for instance in used_instances:
            used_memory += CURDB.groups.get_group(instance.type).card.reqs.instances.memory_guarantee.megabytes() / 1024.

        if used_memory > host.memory:
            failed = True
        if used_memory > host.memory or options.verbose_level > 1:
            print "Host %s, total memory %s, used memory %s" % (host.name, host.memory, used_memory)
            if options.verbose_level > 0:
                for instance in used_instances:
                    print "    %s used %s" % (instance.full_name(), CURDB.groups.get_group(
                        instance.type).card.reqs.instances.memory_guarantee.megabytes() / 1024.)

    return failed


if __name__ == '__main__':
    options = parse_cmd()

    failed = main(options)
    if options.fail_on_error == True and failed == True:
        sys.exit(1)
