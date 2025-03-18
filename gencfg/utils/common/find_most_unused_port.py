#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

from collections import namedtuple
import json

import gencfg
from core.db import CURDB
from core.argparse.parser import ArgumentParserExt
import core.argparse.types


FORBIDDEN_PORTS = dict(
    NETMON=range(48000, 48024),  # GENCFG-2207
    NETMON2=range(11016, 11025),  # GENCFG-2312
    ISS=range(25536, 25537),  # GENCFG-906
    SKYNET=[2399] + range(6881, 7000) + range(10000, 10199),  # GENCFG-2367
)


def get_parser():
    parser = ArgumentParserExt(description="Find most unused port")
    parser.add_argument("--db", type=core.argparse.types.gencfg_db, default=CURDB,
                        help="Optional. Path to db")
    parser.add_argument("--hosts", type=core.argparse.types.hosts, default=None,
                        help="Optional. Search free port only among specified hosts (GENCFG-2136)")
    parser.add_argument("--ignore-groups", type=core.argparse.types.groups, default=None,
                        help="Optional. List of groups to ignore when calculating most suitable port")
    parser.add_argument("-r", "--port-range", type=int, required=True,
                        help="Obligatory. Port range (if you want start one instance, specify 1)")
    parser.add_argument("-s", "--start-port", type=int, default=7000,
                        help="Optional. Start port")
    parser.add_argument("-e", "--end-port", type=int, default=32767,
                        help="Optional. End port")
    parser.add_argument('--preferred-port', type=int, default=None,
                        help='Optional. Preferred port (if specified check if this port if valid first)')
    parser.add_argument("-j", "--json", action='store_true', default=False,
                        help="Optional. Print result in json format")
    parser.add_argument("--strict", action="store_true", default=False,
                        help="Optional. Raise exception, if can not find subrange without instances")

    return parser


def normalize(options):
    def cr(option_name, value, min_value=None, max_value=None):
        if min_value is not None and value < min_value:
            raise Exception("Options %s value %d smaller than minimal %d" % (option_name, value, min_value))
        if max_value is not None and value > max_value:
            raise Exception("Options %s value %d greater than maximal %d" % (option_name, value, max_value))

    cr("port_range", options.port_range, 1, 1000)
#    cr("start_port", options.start_port, 1025, 32767)
#    cr("end_port", options.end_port, 1025, 32767)

    if options.end_port - options.start_port - options.port_range < 0:
        raise Exception("Failed constraing (end_port - start_port - port_range) >= 0")

    if options.hosts is not None:
        options.hosts = set(options.hosts)

    if options.ignore_groups is None:
        options.ignore_groups = []
    options.ignore_groups = {x.card.name for x in options.ignore_groups}


def main(options):
    MAX_PORT = 65536
    by_port_counts = [0] * MAX_PORT

    forbidden_ports = sum([x for x in FORBIDDEN_PORTS.itervalues()], [])
    for forbidden_port in forbidden_ports:
        by_port_counts[forbidden_port] = 1000000

    # add some fake instances to starting port of each group
    if options.hosts is not None:
        all_instances = sum([options.db.groups.get_host_instances(x) for x in options.hosts], [])
        all_groups = [options.db.groups.get_group(x) for x in set(x.type for x in all_instances)]

        group_extra_ports = dict()
        group_juggler_port = dict()
        group_golovan_port = dict()
        for group in all_groups:
            if group.card.legacy.funcs.instancePort.startswith('new'):
                group_extra_ports[group.card.name] = 8
            else:
                group_extra_ports[group.card.name] = 1
            if group.card.properties.monitoring_juggler_port is not None:
                group_juggler_port[group.card.name] = group.card.properties.monitoring_juggler_port
            else:
                group_juggler_port[group.card.name] = 0
            if group.card.properties.monitoring_golovan_port is not None:
                group_golovan_port[group.card.name] = group.card.properties.monitoring_golovan_port
            else:
                group_golovan_port[group.card.name] = 0

        for instance in all_instances:
            if instance.type in options.ignore_groups:
                continue

            for shift in range(group_extra_ports[instance.type]):
                by_port_counts[instance.port + shift] += 1
            by_port_counts[group_juggler_port[instance.type]] += 1
            by_port_counts[group_golovan_port[instance.type]] += 1
    else:
        # update ghi cache to get all instances from it
        for group in options.db.groups.get_groups():
            if group.ghi.groups.get(group.card.name).instances is None:
                group.get_instances()

        # get number of instances on every port
        for instance in options.db.groups.ghi.instances.values():
            by_port_counts[instance.port] += 1

        # RX-86 add ports from monitoring
        if group.card.properties.monitoring_juggler_port is not None:
            by_port_counts[group.card.properties.monitoring_juggler_port] += len(group.get_instances())
        if group.card.properties.monitoring_golovan_port is not None:
            by_port_counts[group.card.properties.monitoring_golovan_port] += len(group.get_instances())

        FK = 10
        for group in options.db.groups.get_groups():
            pfunc = group.card.legacy.funcs.instancePort
            if pfunc == 'default':
                by_port_counts[8041] += FK
            elif pfunc.startswith('old'):
                by_port_counts[int(pfunc[3:])] += FK
            elif pfunc.startswith('new'):
                by_port_counts[int(pfunc[3:])] += FK
            else:
                raise Exception("Unknown instance port func <%s>" % pfunc)

    # ============================== GENCFG-1699 START ====================================================
    for port in range(25530, 25545):
        by_port_counts[port] += 100000
    # ============================== GENCFG-1699 FINISH ===================================================

    for port in range(1, MAX_PORT):
        by_port_counts[port] += by_port_counts[port - 1]

    candidate_counts = [0] * MAX_PORT
    for port in range(options.start_port, options.end_port - options.port_range + 1):
        candidate_counts[port] = by_port_counts[port + options.port_range - 1] - by_port_counts[port - 1]
    candidate_counts = candidate_counts[options.start_port:(options.end_port - options.port_range + 1)]

    minimal_intersection = min(candidate_counts)
    if options.strict is True and minimal_intersection > 0:
        raise Exception("Can not find range of %d port without any instance inside" % options.port_range)

    ResultType = namedtuple("ResultType", ["port", "intersection", "free_range"])

    if options.preferred_port is not None:
        sp = options.preferred_port - options.start_port
        ep = sp + 8
        if set(candidate_counts[sp:ep]) == set([0]):
            return ResultType(port=options.preferred_port, intersection=0, free_range=0)

    # find longest continous port range with minimal intersection
    best_candidate, best_candidate_range = None, 0
    current_candidate, current_candidate_range = None, 0
    for i in range(len(candidate_counts)):
        if candidate_counts[i] == minimal_intersection:
            if current_candidate is None:
                current_candidate, current_candidate_range = i, 1
            else:
                current_candidate_range += 1
        else:
            if current_candidate is not None:
                if current_candidate_range > best_candidate_range:
                    best_candidate, best_candidate_range = current_candidate, current_candidate_range
                current_candidate, current_candidate_range = None, 0
    if current_candidate is not None:
        if current_candidate_range > best_candidate_range:
            best_candidate, best_candidate_range = current_candidate, current_candidate_range

    return ResultType(port=best_candidate + options.start_port + best_candidate_range / 2,
                      intersection=minimal_intersection, free_range=best_candidate_range)


def jsmain(d):
    options = get_parser().parse_json(d)
    normalize(options)
    return main(options)


if __name__ == '__main__':
    options = get_parser().parse_cmd()
    normalize(options)

    result = main(options)

    if options.json:
        print json.dumps({
            'port': result.port,
            'intersection': result.intersection,
            'range_low': result.port,
            'range_high': result.port + options.port_range - 1,
        }, indent=4)
    else:
        print "Found port %s with %d other instances in range [%s, %s) (get medium from %d interval))" % \
              (result.port, result.intersection, result.port, result.port + options.port_range, result.free_range)
