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
    parser = ArgumentParser(description="Show instances power")
    parser.add_argument("-i", "--instances", dest="instances", type=argparse_types.instances, required=True,
                        help="Obligatory. List of instances")

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    options = parser.parse_args()

    options.instances = map(lambda x: (x, 1.0), options.instances)

    return options


def main(options):
    gcounts = defaultdict(int)
    pcounts = defaultdict(float)
    for instance, coeff in options.instances:
        gcounts[instance.type] += 1
        pcounts[instance.type] += instance.power * coeff

    result = []
    for k in sorted(gcounts.keys()):
        result.append((k, gcounts[k] / float(len(CURDB.groups.get_group(k).get_instances())), pcounts[k]))

    return result


if __name__ == '__main__':
    options = parse_cmd()

    result = main(options)
    for groupname, instance_percents, power_percents in result:
        print "%s: instances %.2f power %.2f" % (groupname, instance_percents * 100, power_percents * 100)
