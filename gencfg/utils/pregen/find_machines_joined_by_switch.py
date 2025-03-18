#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

from optparse import OptionParser
from collections import defaultdict

import gencfg
from core.db import CURDB


def parse_cmd():
    usage = "Usage %prog [options]"
    parser = OptionParser(usage=usage)

    parser.add_option("-n", "--hosts-per-group", type="int", dest="hosts_per_group",
                      help="obligatory. Hosts in single group")
    parser.add_option("-p", "--power", type="float", dest="power",
                      help="obligatory. Total power required.")
    parser.add_option("-f", "--host-filter", type="str", dest="host_filter", default=None,
                      help="optional. Search host by filter.")
    parser.add_option("-g", "--groups", dest="groups", default=None,
                      help="obligatory. Comma-separated list of groups")
    parser.add_option("-v", "--use_slaves", action="store_true", dest="use_slaves", default=False,
                      help="optional. Use hosts with slaves")

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    (options, args) = parser.parse_args()

    if options.hosts_per_group is None:
        raise Exception("Option --hosts-per-group is obligatory")
    if options.power is None:
        raise Exception("Option --power is obligatory")
    if options.groups is None:
        raise Exception("Option --groups is obligatory")
    if len(args):
        raise Exception("Unparsed args: %s" % args)

    options.host_filter = eval(options.host_filter) if options.host_filter is not None else None

    return options


def _find_switch(data, required_power=None, find_best=True, N=None):
    candidate = None

    #    print data.items()
    for (power, switch), hosts in data.items():
        if required_power is not None and power != required_power:
            continue
        if candidate is None:
            if len(hosts) > 0:
                candidate = hosts
            continue
        if find_best and len(hosts) > len(candidate):
            candidate = hosts
            continue
        if not find_best and len(candidate) > len(hosts) > 0:
            candidate = hosts
            continue

    if candidate is None:
        raise Exception("Can not find hosts with power %s" % required_power)
    #    print "Candidate", candidate[0].switch

    result = []

    for i in range(min(N, len(candidate))):
        result.append(candidate.pop())
    #    print "Result", len(result)

    return result


def find_machines(options):
    # FIXME: reuse code from find_machines.py
    groups = [CURDB.groups.get_group(group) for group in options.groups.split('+')]
    hosts = reduce(lambda x, y: x | y, (group.hosts for group in groups), set())

    if options.host_filter:
        hosts = set(filter(lambda x: options.host_filter(x), hosts))

    if not options.use_slaves:
        slave_hosts = set()
        for group in groups:
            for sgroup in group.slaves:
                # WTF check?
                if sgroup.hosts != group.hosts:
                    slave_hosts |= set(sgroup.hosts)
        hosts -= slave_hosts

    hosts_by_power_and_switch = defaultdict(list)
    for host in hosts:
        hosts_by_power_and_switch[(host.power, host.switch)].append(host)

    result = []
    while options.power > 0:
        step_hosts = []
        find_best = True
        power = None

        while len(step_hosts) < options.hosts_per_group:
            step_hosts.extend(
                _find_switch(hosts_by_power_and_switch, power, find_best, options.hosts_per_group - len(step_hosts)))
            find_best = False
            power = step_hosts[-1].power

        result.extend(step_hosts)
        options.power -= power
        print "Added group with power %s" % power

    return result


if __name__ == '__main__':
    options = parse_cmd()
    machines = find_machines(options)

    print "Chosed %d hosts with %f power: %s" % (
    len(machines), sum(x.power for x in machines), ",".join(x.name for x in machines))
