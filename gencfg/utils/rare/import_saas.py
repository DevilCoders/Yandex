#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))

import re
import math

from argparse import ArgumentParser

import gencfg
from core.db import CURDB
import core.argparse.types as argparse_types
import optimizers.dynamic.main as update_fastconf

ACTIONS = ["show", "update"]


class TSaasGroup(object):
    REGEX = ("([\w\-.]+)\n"
             "INS:\s+MSK=(\d+)\s+SAS=(\d+)\s+MAN=(\d+)\n"
             "CPU:\s+avg=[0-9.]+\s+95th=([0-9.]+)\s+max=[0-9.]+\n"
             "MEM:\s+avg=[0-9.]+\s+95th=([0-9.]+)\s+max=[0-9.]+\s\(KiB\)\n"
             "IOR:\s+avg=[0-9.]+\s+95th=([0-9.]+)\s+tmax=[0-9.]+\s\(KiB\)\n"
             "IOW:\s+avg=[0-9.]+\s+95th=([0-9.]+)\s+tmax=[0-9.]+\s\(KiB\)")

    def __init__(self, s, position):
        p = re.compile(TSaasGroup.REGEX)

        m = p.match(s, position)
        if not m:
            raise Exception("Can not parse Saas group at position %s (%s..." % (position, s[position:position + 100]))

        self.saas_name = m.group(1)
        self.gencfg_saas_name = re.sub('-', '_', self.saas_name).upper()
        gencfg_group_names = ["MSK_FOL_SAAS_" + self.gencfg_saas_name + "_BASE",
                              "MSK_UGRB_SAAS_" + self.gencfg_saas_name + "_BASE",
                              "SAS_SAAS_" + self.gencfg_saas_name + "_BASE",
                              "MAN_SAAS_" + self.gencfg_saas_name + "_BASE"]
        self.gencfg_groups = map(lambda x: CURDB.groups.get_group(x),
                                 filter(lambda x: CURDB.groups.has_group(x), gencfg_group_names))

        # add required number of instances in every dc
        self.instances = {
            "msk": int(m.group(2)),
            "sas": int(m.group(3)),
            "man": int(m.group(4)),
        }

        # m.group(5) is number of cores, used by every instance in average. So to get our power units we should multiply it by power of one core
        self.cpu = int((sum(self.instances.values())) * float(m.group(5)) * 35.0)

        # memory in gigabytes
        self.memory = float(m.group(6)) / 1024. / 1024.

        self.ior = float(m.group(7)) / 1024.
        self.iow = float(m.group(8)) / 1024.

        self.last_position = position + len(m.group(0))

    def __repr__(self):
        return """Saas group %s:
    gencfg groups: %s
    instances: %s msk %s sas %s man
    cpu: %.2f
    memory: %.2f Gb
    ior: %.2f Mb/sec
    iow: %.2f Mb/sec""" % (self.saas_name, ",".join(map(lambda x: x.name, self.gencfg_groups)), self.instances["msk"],
                           self.instances["sas"], self.instances["man"],
                           self.cpu, self.memory, self.ior, self.iow)


def parse_cmd():
    parser = ArgumentParser(description="Import saas groups from marchael format RCPS-2884")
    parser.add_argument("-a", "--action", type=str, required=True,
                        choices=ACTIONS,
                        help="Obligatory. Action to execute")
    parser.add_argument("-i", "--input-file", type=str, required=True,
                        help="Obligatory. Input file name")
    parser.add_argument("-f", "--filter", type=argparse_types.pythonlambda, default=lambda x: True,
                        help="Optional. Filter on groups to show/update")

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    options = parser.parse_args()

    return options


def allocate_in_dynamic_for_location(saas_group, location):
    MAPPING = {
        "msk": "MSK_UGRB",
        "sas": "SAS",
        "man": "MAN",
    }
    util_location = MAPPING.get(location, None)
    if location is None:
        raise Exception("Unknown location %s" % util_location)

    util_params = {
        "master_group": "ALL_DYNAMIC",
        "action": "add",
        "group": "%s_SAAS_%s_BASE" % (util_location, saas_group.gencfg_saas_name),
        "owners": "marchael,amich",
        "description": "Automatically generated from saas group %s from task RCPS-2884" % saas_group.saas_name,
        "location": util_location,
        "itype": "rtyserver",
        "ctype": "prestable" if location == "SAS" else "prod",
        "prj": [saas_group.saas_name],
        "metaprj": "saas",
        "min_power": saas_group.cpu,
        "memory": "%.2f Gb" % saas_group.memory,
        "min_replicas": saas_group.instances[location],
        "max_replicas": saas_group.instances[location],
    }
    result = update_fastconf.jsmain(util_params, CURDB, from_cmd=False)

    return result


def main(options):
    data = open(options.input_file).read()
    saas_groups = []

    position = 0
    while position < len(data):
        saas_group = TSaasGroup(data, position)
        position = saas_group.last_position + 1
        saas_groups.append(saas_group)

    saas_groups = filter(options.filter, saas_groups)

    if options.action == "show":
        for saas_group in saas_groups:
            print saas_group
    elif options.action == "update":
        for saas_group in saas_groups:
            if len(saas_group.gencfg_groups) > 0:
                print "Can not update saas group %s: already have gencfg group %s" % (
                saas_group.saas_name, ",".join(map(lambda x: x.name, self.gencfg_groups)))

            print "Creating gencfg groups for %s" % saas_group.saas_name
            for location in ["msk", "sas", "man"]:
                group = allocate_in_dynamic_for_location(saas_group, location)
                print "    Created group %s with %d instances" % (group.card.name, len(group.get_instances()))

        CURDB.update()
    else:
        raise Exception("Unknown action %s" % options.action)


if __name__ == '__main__':
    options = parse_cmd()
    main(options)
