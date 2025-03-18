#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

from argparse import ArgumentParser

import gencfg
from core.db import CURDB
import core.argparse.types as argparse_types


def parse_cmd():
    parser = ArgumentParser(description="Replace selected instances by free ones")
    parser.add_argument("-i", "--intlookup", type=argparse_types.intlookup, default=None,
                        help="Optional. Inlookup to work with (use group intlookup if this option is omitted)")
    parser.add_argument("-g", "--group", type=argparse_types.group, default=None,
                        help="Optiona. Group with free instances (use intlookup base_type is this option is omitted)")
    parser.add_argument("-s", "--hosts", type=argparse_types.hosts, required=True,
                        help="Obligatory. Hosts list to replace")
    parser.add_argument("-f", "--flt", type=argparse_types.pythonlambda, default=lambda x: True,
                        help="Optional. Filter free instances")

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    options = parser.parse_args()

    return options


def normalize(options):
    if options.intlookup is None and options.group is None:
        raise Exception("You must specify at least one of --intlookup --group option")

    if options.intlookup is None:
        if len(options.group.card.intlookups) != 1:
            raise Exception("Group %s must contain exactly one intlookup" % options.group.card.name)
        options.intlookup = CURDB.intlookups.get_intlookup(options.group.card.intlookups[0])

    if options.group is None:
        options.group = CURDB.groups.get_group(options.intlookup.base_type)


def run_with_group(options, group):
    free_instances = list(set(group.get_instances()) - set(group.get_busy_instances()))
    free_instances = filter(options.flt, free_instances)
    free_instances = filter(lambda x: x.host not in options.hosts, free_instances)

    to_replace_instances = filter(lambda x: x.host in options.hosts, group.get_busy_instances())

    if len(free_instances) < len(to_replace_instances):
        raise Exception("Not enough instances to replace: have %d free, while need to replace %d" % (len(free_instances), len(to_replace_instances)))

    def f(instance):
        if instance.host in options.hosts and instance.type == group.card.name:
            free_instance = free_instances.pop()
            instance.swap_data(free_instance)

    options.intlookup.for_each(f, run_base=True, run_int=True)
    options.intlookup.mark_as_modified()

    CURDB.intlookups.update(smart=True)


def main(options):
    all_groups = set(map(lambda x: x.type, options.intlookup.get_used_instances()))
    all_groups = map(lambda x: CURDB.groups.get_group(x), all_groups)

    for group in all_groups:
        run_with_group(options, group)


if __name__ == '__main__':
    options = parse_cmd()
    normalize(options)
    main(options)
