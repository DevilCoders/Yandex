#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))
import gencfg

from argparse import ArgumentParser

import core.argparse.types as argparse_types


def parse_cmd():
    parser = ArgumentParser(description="Dump maximum unworking limits for groups")

    parser.add_argument("-g", "--groups", dest="groups", type=argparse_types.groups, required=True,
                        help="Comma-separated list of groups")
    parser.add_argument("-o", "--output-file", dest="output_file", type=str, required=True,
                        help="Path to output file")

    if len(sys.argv) == 1:
        sys.argv.append('-h')

    options = parser.parse_args()

    return options


def main(options):
    with open(options.output_file, 'w') as f:
        for group in options.groups:
            print >> f, "%s %s" % (group.card.name, len(group.getHosts()) / 100 + 1)
            for slavegroup in group.slaves:
                print >> f, "%s %s" % (slavegroup.card.name, len(slavegroup.getHosts()) / 100 + 1)


if __name__ == '__main__':
    options = parse_cmd()
    main(options)
