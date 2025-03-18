#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

from argparse import ArgumentParser

import gencfg
import core.argparse.types as argparse_types
from core.instances import FakeInstance
from core.hosts import FakeHost


def parse_cmd():
    parser = ArgumentParser(description="Script to show instances tags")

    parser.add_argument("-i", "--instances", dest="instances", type=str, required=True,
                        help="Obligatory. Comma-separated instances in format host:port")
    parser.add_argument("-s", "--searcherlookup", type=argparse_types.stripped_searcherlookup, dest="searcherlookup",
                        required=True,
                        help="Optional. Searcherlookup with data")

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    options = parser.parse_args()

    options.instances = options.instances.split(',')

    return options


if __name__ == '__main__':
    options = parse_cmd()

    for instance in options.instances:
        host, _, port = instance.partition(':')
        port = int(port)

        fake_instance = FakeInstance(FakeHost(host), port)
        print "%s: %s" % (instance, ' '.join(options.searcherlookup.instances[fake_instance]))
