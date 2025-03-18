#!/skynet/python/bin/python
import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import math
from collections import defaultdict
from argparse import ArgumentParser

import gencfg

from core.db import CURDB
import core.argparse.types as argparse_types
from gaux.aux_decorators import gcdisable


def parse_cmd():
    parser = ArgumentParser(description="Check consistencty of custom instances power")
    parser.add_argument("-g", "--groups", type=argparse_types.groups,
                        help="Optional. Comma-separated groups to check (use \"ALL\" for all groups)")
    parser.add_argument("-c", "--sas-config", dest="sas_config", type=argparse_types.sasconfig, required=False,
                        help="Optional. Sas config")

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    options = parser.parse_args()

    if options.groups is None and options.sas_config is None:
        raise Exception("You must specify at least one of --groups --sas-config option")

    return options

@gcdisable
def main(options):
    instances = []
    if options.sas_config:
        intlookups = map(lambda x: CURDB.intlookups.get_intlookup(x.intlookup), options.sas_config)
        for intlookup in intlookups:
            instances.extend(intlookup.get_used_base_instances())
    for group in options.groups:
        instances.extend(group.get_instances())

    failed = False
    mp = defaultdict(float)
    for instance in instances:
        mp[(CURDB.groups.get_group(instance.type), instance.host)] += instance.power

    for group, host in mp:
        real_power = sum(group.funcs.instancePower(CURDB, host, group.funcs.instanceCount(CURDB, host)))
        if math.fabs(real_power - mp[(group, host)]) > 0.1:
            print "Group %s, host %s: %s real and %s custom" % (group.card.name, host.name, mp[(group, host)], real_power)
            failed = True

    return failed


if __name__ == '__main__':
    options = parse_cmd()
    ret = main(options)
    sys.exit(ret)
