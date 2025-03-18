#!/skynet/python/bin/python

import os
import sys
from argparse import ArgumentParser

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))
from collections import defaultdict

import gencfg
import core.argparse.types as argparse_types
from tools.analyzer.runner import Runner
import tools.analyzer.functions


def parse_cmd():
    parser = ArgumentParser(description="Script for check currently unworking instances and hosts")
    parser.add_argument("-d", "--detailed", action="store_true", dest="detailed", default=False,
                        help="choose detailed")
    parser.add_argument("-b", "--base", action="store_true", dest="base", default=False,
                        help="check base")
    parser.add_argument("-n", "--int", action="store_true", dest="int", default=False,
                        help="check int")
    parser.add_argument("-i", "--intlookups", type=argparse_types.intlookups, dest="intlookups", default=None,
                        help="comma-separated list of intlookups")
    parser.add_argument("-g", "--groups", type=argparse_types.groups, dest="groups", default=None,
                        help="Optional. Comma-separated list of gruops")
    parser.add_argument("-t", "--timeout", type=int, dest="timeout", default=30,
                        help="remote command timeout")

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    options = parser.parse_args()

    if options.intlookups is None and options.groups is None:
        raise Exception("You must specify intlookups or groups")

    if options.intlookups is not None:
        if options.base == False and options.int == False:
            raise Exception("At least on of [\"-b\" , \"-i\"] option must be set")

    return options


if __name__ == '__main__':
    options = parse_cmd()

    instances = []
    if options.groups is not None:
        for group in options.groups:
            if len(group.card.intlookups):
                instances.extend(group.get_busy_instances())
            else:
                instances.extend(group.get_instances())
    if options.intlookups is not None:
        for intlookup in options.intlookups:
            if options.base:
                instances.extend(intlookup.get_base_instances())
            if options.int:
                instances.extend(intlookup.get_int_instances())

    runner = Runner()
    skynet_failure_instances, result = runner.run_on_instances(instances,
                                                               [tools.analyzer.functions.instance_is_running],
                                                               timeout=options.timeout)

    unworking_instances = dict(
        filter(lambda (x, y): 'instance_is_running' in y and y['instance_is_running'] == 0, result.items()))
    print "%d skynet failed instances, %d unworking instances (%d total)" % (len(skynet_failure_instances), len(unworking_instances), len(result.keys()))

    # show unworking instances
    nonworking_by_queue = defaultdict(list)
    for instance in skynet_failure_instances:
        nonworking_by_queue[instance.host.queue].append(instance)
    for queue in nonworking_by_queue:
        print "Queue %s: %d unworking+failed" % (queue, len(nonworking_by_queue[queue]))
    if options.detailed:
        print "Unworking instances: %s" % ' '.join(map(lambda x: '%s:%s' % (x.host.name, x.port), unworking_instances))
        print "Skynet failed hosts: %s" % ' '.join(list(set(map(lambda x: x.host.name, skynet_failure_instances))))
