#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

from argparse import ArgumentParser

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

    options.instances = map(lambda x: (x, {}), options.instances)

    return options


def main(options, db=CURDB):
    total_power = 0.
    total_memory = 0.

    for instance, iprops in options.instances:
        group = db.groups.get_group(instance.type)
        n = len(group.get_host_instances(instance.host))

        total_power += instance.power * iprops.get('coeff', 1.0)
        total_memory += instance.host.memory / n * iprops.get('coeff', 1.0)

    return total_power, total_memory


if __name__ == '__main__':
    options = parse_cmd()
    total_power, total_memory = main(options)
    print "Total power %s, total memory %s" % (total_power, total_memory)
