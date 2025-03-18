#!/skynet/python/bin/python
"""
    This utility moves pre-defined instances to free machines, thus reducint number of instances on most overloaded machines
"""

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import gencfg
from core.db import CURDB
from core.instances import TMultishardGroup
from core.argparse.parser import ArgumentParserExt
import core.argparse.types


def get_parser():
    parser = ArgumentParserExt(description="Move some instances to free machines")
    parser.add_argument("-g", "--group", type=core.argparse.types.group, required=True,
                        help="Obligatory. Group name to process.")
    parser.add_argument("-i", "--instances", type=core.argparse.types.instances, required=True,
                        help="Obligatory. Comma-separated list of instances in format <hosname>:<port>")
    parser.add_argument("-s", "--hosts", type=core.argparse.types.hosts, default=None,
                        help="Optional. List of hosts to get new instances from (if not specified all free hosts will be used)")

    return parser


def normalize(options):
    if options.hosts is None:
        all_hosts = set(options.group.getHosts())
        used_intlookups = map(lambda y: CURDB.intlookups.get_intlookup(y), options.group.card.intlookups)
        used_hosts = sum(map(lambda x: x.get_used_instances(), used_intlookups), [])
        used_hosts = set(map(lambda x: x.host, used_hosts))
        options.hosts = list(all_hosts - used_hosts)

    if len(options.hosts) < len(options.instances):
        raise Exception("Have only <%d> hosts when we want to replace <%d> instances" % (len(options.hosts), len(options.instances)))


def main(options):
    def _instance_replacement(bad_instance):
        if bad_instance not in options.instances:
            return

        good_host = options.hosts.pop()
        good_instances = options.group.get_host_instances(good_host)

        # fix power of instances
        if len(good_instances) > 1:
            good_instances_power = sum(map(lambda x: x.power, good_instances))
            if good_instances_power < bad_instance.power:
                raise Exception("Power of all instances in replacement host <%s> is <%d>, which is smaller than power of <%s>" % (good_host.power, good_instances_power, bad_instance.full_name()))

            good_instances[0].power = bad_instance.power
            others_power = (good_instances_power - bad_instance.power) / (len(good_instances) - 1)
            for instance in good_instances[1:]:
                instance.power = others_power

        return good_instances[0]

    for intlookup in map(lambda x: CURDB.intlookups.get_intlookup(x), options.group.card.intlookups):
        intlookup.replace_instances(_instance_replacement, run_base=True, run_int=True, run_intl2=True)
        intlookup.mark_as_modified()
    options.group.mark_as_modified()

    CURDB.update(smart=True)


if __name__ == '__main__':
    options = get_parser().parse_cmd()

    normalize(options)

    main(options)
