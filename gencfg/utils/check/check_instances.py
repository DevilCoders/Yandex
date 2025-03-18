#!/skynet/python/bin/python

import os
import sys
import time

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

from collections import defaultdict
from argparse import ArgumentParser
import multiprocessing

import gencfg
from core.db import CURDB
from core.igroups import CIPEntry
from utils.common.find_most_unused_port import FORBIDDEN_PORTS
from gaux.aux_decorators import gcdisable
from gaux.aux_colortext import red_text


FP = {}
for name, ports in FORBIDDEN_PORTS.iteritems():
    for port in ports:
        FP[port] = name


def check_forbiden_ports(hostname, port):
    if port in FP:
        return [red_text('Interception with {} on {}:{}'.format(FP[port], hostname, port))]
    return []


class TPortHolderInstance(object):
    def __init__(self, name):
        self.name = name

    def full_name(self):
        return 'PORT_HOLDER_FOR_{}'.format(self.name)


def parse_cmd():
    parser = ArgumentParser(description='Check for instances of differnet groups do not intersect by port')
    parser.add_argument('--only-dynamic', action='store_true', default=False,
                help='Optional. Check only dynamic groups')
    parser.add_argument('--detailed-report', action='store_true', default=False,
                help='Optional. Detailed report on intersections')
    parser.add_argument('--workers', type=int, default=1,
                help='Optional. Run multiple workers simultaneously')

    options = parser.parse_args()

    return options


def check_instances_ports(args):
    only_dynamic, fast, worker_id, worker_count = args

    instances_by_host_port = defaultdict(list)
    if fast:
        instances_by_host_port = defaultdict(lambda: defaultdict(int))

    for group in CURDB.groups.get_groups():
        if only_dynamic:  # check intersection in dynamic groups only
            if (group.card.properties.background_group == False) and (group.card.master is None or group.card.master.card.name != 'ALL_DYNAMIC'):
                continue

        if group.card.legacy.funcs.instancePort.startswith('new'):
            N = 8
        else:
            N = 1

        for instance in group.get_kinda_busy_instances():
            if hash(instance.host.name) % worker_count != worker_id:
                continue

            if fast:
                host_dict = instances_by_host_port[instance.host.name]
            for i in range(N):
                if fast:
                    host_dict[instance.port+i] += 1
                else:
                    instances_by_host_port[(instance.host.name, instance.port+i)].append(instance)

            # RX-86: check juggler/yasm ports
            for pname in ('monitoring_juggler_port', 'monitoring_golovan_port'):
                pvalue = getattr(group.card.properties, pname, None)
                if (pvalue is not None) and ((pvalue < instance.port) or (pvalue >= instance.port+N)):
                    if fast:
                        host_dict[pvalue] += 1
                    else:
                        instances_by_host_port[(instance.host.name, pvalue)].append(instance)

    result = []
    if fast:
        for hostname, p in instances_by_host_port.iteritems():
            for port, count in p.iteritems():
                if count > 1:
                    result.append(red_text('Multiple instances on {}:{}'.format(hostname, port)))
                result.extend(check_forbiden_ports(hostname, port))
    else:
        for (hostname, port), instances in instances_by_host_port.iteritems():
            if len(instances) > 1:
                result.append(red_text('Multiple instances on {}:{} : {}'.format(hostname, port, ','.join((x.full_name() for x in instances)))))
            result.extend(check_forbiden_ports(hostname, port))

    return result


@gcdisable
def main():
    options = parse_cmd()

    result = []
    if options.workers == 1:
        result.extend(check_instances_ports((options.only_dynamic, not options.detailed_report, 0, 1)))
    else:
        for group in CURDB.groups.get_groups():
            group.get_kinda_busy_instances()

        p = multiprocessing.Pool(options.workers)
        args = [(options.only_dynamic, not options.detailed_report, x, options.workers) for x in range(options.workers)]
        for subresult in p.map(check_instances_ports, args):
            result.extend(subresult)

    failed = len(result)
    if result:
         print '\n'.join(result)

    # check custom instances power
    for group in CURDB.groups.get_groups():
        if not group.custom_instance_power_used:
            continue
        cip_instances = set(group.custom_instance_power.keys())
        group_instances = set(map(lambda x: CIPEntry(x), group.get_instances()))
        notfound_instances = cip_instances - group_instances
        if len(notfound_instances) > 0:
            print "Group %s has CIP instances with wrong host/port: %s" % (group.card.name, ",".join(map(lambda x: "%s:%s" % (x.host.name, x.port), notfound_instances)))
            failed = True

    if failed:
        sys.exit(1)

if __name__ == '__main__':
    main()
