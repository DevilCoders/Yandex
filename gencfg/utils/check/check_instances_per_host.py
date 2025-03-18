#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

from collections import defaultdict
from argparse import ArgumentParser

import gencfg
import core.argparse.types as argparse_types


def parse_cmd():
    parser = ArgumentParser(description="Check if number of instances per host in specified range")
    parser.add_argument("-i", "--intlookups", type=argparse_types.intlookups, required=True,
                        help="Obligatory. List of intlookups")
    parser.add_argument("-m", "--min-instances", type=int, default=0,
                        help="Optional. Minimal number of instances per host")
    parser.add_argument("-M", "--max-instances", type=int, default=100000,
                        help="OPtional. Maximum number of instances per host")

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    options = parser.parse_args()

    return options


def main(options):
    instances_by_host = defaultdict(list)

    for instance in sum(map(lambda x: x.get_used_base_instances(), options.intlookups), []):
        instances_by_host[instance.host].append(instance)

    failed = False
    for host in instances_by_host:
        if len(instances_by_host[host]) < options.min_instances:
            print "Host %s has %d instances, when should have at least %d: %s" % (host.name, len(instances_by_host[host]), options.min_instances, ",".join(map(lambda x: x.name, instances_by_host[host])))
            failed = True
        if len(instances_by_host[host]) > options.max_instances:
            print "Host %s has %d instances, when should have at most %d: %s" % (host.name, len(instances_by_host[host]), options.max_instances, ",".join(map(lambda x: x.name(), instances_by_host[host])))
            failed = True

    return failed


if __name__ == '__main__':
    options = parse_cmd()
    retcode = main(options)
    sys.exit(int(retcode))
