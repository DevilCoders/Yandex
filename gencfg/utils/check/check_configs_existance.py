#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

from argparse import ArgumentParser

import gencfg
import config
from core.db import CURDB
import core.argparse.types as argparse_types


def parse_cmd():
    parser = ArgumentParser(description="Add extra replicas to intlookup (from free instances)")
    parser.add_argument("-i", "--itype", type=str, required=True,
                        help="Obligatory. Itype of instances")
    parser.add_argument("-x", "--exclude-groups", type=argparse_types.groups, default=[],
                        help="Optional. List of groups to exclude from checking")

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    options = parser.parse_args()

    return options


def main(options):
    failed = 0
    for group in CURDB.groups.get_groups():
        if group.card.tags.itype != options.itype:
            continue
        if group in options.exclude_groups:
            continue

        for instance in group.get_busy_instances():
            configname = os.path.join(config.MAIN_DIR, config.WEB_GENERATED_DIR, 'all',
                                      '%s:%s.cfg' % (instance.host.name.split('.')[0], instance.port))
            if not os.path.exists(configname):
                print "!!! ERROR !!! Group %s, instance %s config not found" % (group.card.name, '%s:%s' % (instance.host.name, instance.port))
                failed = 1

    return failed


if __name__ == '__main__':
    options = parse_cmd()
    status = main(options)

    sys.exit(status)
