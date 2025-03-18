#!/skynet/python/bin/python

import os
import sys
import math

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '../ytils')))
from cStringIO import StringIO
import copy
from collections import defaultdict

import gencfg
from core.resources import TResources
from core.argparse.parser import ArgumentParserExt
from core.db import CURDB
from gaux.aux_utils import indent, freq, prompt
from gaux.aux_shared import calc_source_hosts
import core.argparse.types as argparse_types

import utils.common.calc_power_expression


def get_parser():
    parser = ArgumentParserExt(
        description="DEPRECATED TOOL. Find machines that satisfy specified conditions (for allocating hosts from reserved mainly)")
    parser.add_argument("-i", "--instances", type=argparse_types.primus_int_count, dest="min_instances", default=0,
                        help="optional. Total number of instances required.")
    parser.add_argument("-p", "--power", type=str, dest="min_power", default="0",
                        help="optional. Total power required.")
    parser.add_argument("-I", "--min-skip-instances", type=int, default=0,
                        help="optional. Min number of instances left at any DC fail.")
    parser.add_argument("-P", "--min-skip-power", type=float, default=0,
                        help="optional. Min instances power left at any DC fail. ")
    parser.add_argument("-d", "--distribution", type=str,
                        help="optional. Power distribution between DCS")
    parser.add_argument("-t", "--instances-type", type=argparse_types.group, default="SAS_RESERVED",
                        help="Optional. Type of instances. The type of instances determine how mach memory required for one instance" + \
                             " (in othe words, this option determines how many instances will place on one machine).")
    parser.add_argument("-f", "--host-filter", type=argparse_types.pythonlambda, default=None,
                        help="optional. Search host by filter.")
    parser.add_argument("-s", "--opt-switch-load-granularity", type=int, dest="swload_gran", default=0,
                        help="optional, int. Switch load optimization granularity.")
    parser.add_argument("-S", "--max-hosts-per-switch", type=int, default=0,
                        help="optional, int. Max hosts per switch.")
    parser.add_argument("-Q", "--max-hosts-per-queue", type=int, default=0,
                        help="optional, int. Max hosts per queue.")
    parser.add_argument("-g", "--src-groups", type=argparse_types.groups, default=None,
                        help="Optional. Comma-separated list of groups (get reserved groups if not specified)")
    parser.add_argument('-x', '--skip-groups', type=argparse_types.groups, default=None,
                        help='Optional. Skip all hosts, belonging to any of specified groups')
    parser.add_argument("-l", "--use_slaves", action="store_true", default=False,
                        help="optional. Use hosts with slaves")
    parser.add_argument("-v", "--verbose", action="store_true", default=False,
                        help="optional. Explain what is being done")
    parser.add_argument("--move-to-group", type=argparse_types.group, default=None,
                        help="Optional. Move allocated machines to specified group")
    parser.add_argument("--confirm", action="store_true", default=False,
                        help="Optional. When options <--move-to-group> enabled, require extra confirmation to move hosts")
    parser.add_argument("--min-host-resources", type=argparse_types.gencfg_resources, default=None,
                        help="Optional. Specify minimal host resources as <power=12,memory=1 Gb,hdd=2 Gb,ssd=3 Gb,net=4 Gb>")
    parser.add_argument("--not-deprecated", default=False, required=True, help="Required. If you want to use this deprecated tool you need to add this argument")

    return parser


def normalize(options):
    options.min_power = utils.common.calc_power_expression.jsmain({"expression": options.min_power})

    if options.src_groups is None:
        options.src_groups = CURDB.groups.get_reserved_groups()

    options.power_distribution = None
    options.instance_distribution = None

    if options.min_instances is None and options.min_power is None:
        raise Exception("You should specify at least one of --instances or --power option")

    if options.distribution is not None:
        options.distribution = eval(options.distribution)
        if not isinstance(options.distribution, dict):
            raise Exception('--distribution should be a dictionary')
        if options.power > 0 and options.instances > 0:
            raise Exception("Distribution parameter is only allowed when either power = 0 or instances = 0")
        if options.power > 0:
            s = sum(options.distribution.values(), .0)
            options.power_distribution = {}
            for key, value in options.distribution:
                options.power_distribution[key] = (value / s) * options.power
        if options.instances > 0:
            s = sum(options.distribution.values(), .0)
            instance_distribution = {}
            for key, value in options.distribution.items():
                instance_distribution[key] = (value / s) * options.instances

    if options.instances_type is None:
        # single instance with whole host power
        options.instance_power_func = lambda host: [host.power]
    else:
        instanceCount = lambda x: options.instances_type.funcs.instanceCount(CURDB, x)
        instancePower = lambda x, y: options.instances_type.funcs.instancePower(CURDB, x, y)
        options.instance_power_func = lambda host: instancePower(host, instanceCount(host))
    assert options.instance_power_func

    if options.min_skip_power != .0 and \
            (options.instance_distribution is not None or options.power_distribution is not None):
        raise Exception("Options min-skip-power and distribution are mutually exclusive")

    if options.confirm and options.move_to_group is None:
        raise Exception("You can not specify option <--confirm> when not specifiing <--move-to-gorup>")

    options.preferred_hosts = []


# TODO: remove
def _filter_hosts(options):
    assert (isinstance(options.src_groups, list))

    groups = [CURDB.groups.get_group(group) for group in options.src_groups]
    hosts = reduce(lambda x, y: x | y, (group.hosts for group in groups), set())
    if options.verbose:
        print 'Added hosts from source groups. Host count: %d' % len(hosts)

    if not options.use_slaves:
        slave_hosts = set()
        for group in groups:
            for sgroup in group.slaves:
                # WTF check?
                if sgroup.hosts != group.hosts:
                    slave_hosts |= set(sgroup.hosts)
        hosts -= slave_hosts
        if options.verbose:
            print 'Removed slave hosts. Host left: %d' % len(hosts)

    if options.verbose:
        print 'Removed preserved hosts. Host left: %d' % len(hosts)

    # apply filter
    if options.host_filter:
        hosts = set(filter(lambda x: options.host_filter(x), hosts))
        if options.verbose:
            print 'Applied hosts filter. Host left: %d' % len(hosts)

    by_power = defaultdict(int)
    for host in hosts:
        by_power[host.power] += 1
    print 'Hosts count by power %s' % by_power

    dcs = set([host.dc for host in hosts])
    dcs = dict((dc, len([host for host in hosts if host.dc == dc])) for dc in dcs)
    if options.verbose:
        print 'Hosts left by dc: %s' % str(dcs)

    return hosts


# move function to separate utility
def hosts_stats(hosts):
    buf = StringIO()

    def print_stat(name, values, buf):
        if values:
            print >> buf, '%s: Min = %d, Max = %d, Avg = %d' % \
                          (name, min(values), max(values), sum(values) / len(values))

    print_stat('Memory', [host.memory for host in hosts], buf)
    print_stat('CPU', [host.power for host in hosts], buf)
    return buf.getvalue()


class Instance(object):
    def __init__(self, host, n, power):
        self.host = host
        self.n = n
        self.power = power


class Result(object):
    def __init__(self, options):
        self.power = .0
        self.instances = 0
        self.hosts = set()
        self.options = options
        self.dc_power = defaultdict(float)
        self.dc_instances = defaultdict(int)
        self.preferred_hosts = set(options.preferred_hosts)

        self.power_distribution = copy.copy(options.power_distribution)
        if self.power_distribution is not None:
            for key in self.power_distribution:
                assert (self.power_distribution[key] >= 0)
                if self.power_distribution[key] == 0:
                    del self.power_distribution[key]
            if not self.power_distribution:
                self.power_distribution = None

        self.instance_distribution = copy.copy(options.instance_distribution)
        if self.instance_distribution is not None:
            for key in self.instance_distribution:
                assert (self.instance_distribution[key] >= 0)
                if self.instance_distribution[key] == 0:
                    del self.instance_distribution[key]
            if not self.instance_distribution:
                self.instance_distribution = None

    def add_host(self, host):
        assert (host is not None)

        _instances = self.options.instance_power_func(host)
        host_instances = len(_instances)
        host_power = sum(_instances, .0)
        self.power += host_power
        self.instances += host_instances
        self.hosts.add(host)
        self.dc_power[host.dc] += host_power
        self.dc_instances[host.dc] += host_instances
        if self.power_distribution and host.dc in self.power_distribution:
            self.power_distribution[host.dc] -= host_power
            if self.power_distribution[host.dc] <= .0:
                del self.power_distribution[host.dc]
            if not self.power_distribution:
                self.power_distribution = None
        if self.instance_distribution and host.dc in self.instance_distribution:
            self.instance_distribution[host.dc] -= host_instances
            if self.instance_distribution[host.dc] <= 0:
                del self.instance_distribution[host.dc]
            if not self.instance_distribution:
                self.instance_distribution = None

    def filter_by_max_per_location(self, loc, max_limit, hosts):
        n_by_loc = defaultdict(int)
        for host in self.hosts:
            n_by_loc[getattr(host, loc)] += 1

        # if specified type is not equal to group type take into account switch distribution in already allocated hosts
        if options.instances_type is not None and options.instances_type != options.src_groups[0]:
            for host in options.instances_type.getHosts():
                n_by_loc[getattr(host, loc)] += 1

        full_locs = set(loc for loc, load in n_by_loc.items()
                        if load >= max_limit)
        if full_locs:
            hosts = [host for host in hosts if getattr(host, loc) not in full_locs]
        return hosts

    def filter_hosts(self, hosts):
        if self.options.max_hosts_per_switch:
            hosts = self.filter_by_max_per_location('switch', self.options.max_hosts_per_switch, hosts)
        if self.options.max_hosts_per_queue:
            hosts = self.filter_by_max_per_location('queue', self.options.max_hosts_per_queue, hosts)

        dcs = set(host.dc for host in hosts)
        if self.power_distribution:
            dcs &= set(self.power_distribution.keys())
        if self.instance_distribution:
            dcs &= set(self.instance_distribution.keys())
        hosts = [host for host in hosts if host.dc in dcs]

        if hosts and self.options.min_skip_power:
            dcs = set(host.dc for host in hosts)
            max_load = max([self.dc_power.get(dc, .0) for dc in dcs])
            # we should not increase max(dc_power) if we don't meet max_skip_power requirement
            max_host_power = max(host.power for host in hosts)
            max_load_dcs = set(dc for dc in dcs if self.dc_power.get(dc, .0) >= max_load - max_host_power)
            if max(self.power + max_host_power, self.options.min_power) - (
                max_load + max_host_power) < self.options.min_skip_power:
                assert (max_load_dcs & dcs == max_load_dcs)
                if max_load_dcs != dcs:
                    dcs -= max_load_dcs
            hosts = [host for host in hosts if host.dc in dcs]

        if hosts and self.options.min_skip_instances:
            dcs = set(host.dc for host in hosts)
            max_load = max([self.dc_instances.get(dc, 0) for dc in dcs])
            max_load_dcs = set(dc for dc in dcs if self.dc_instances.get(dc, 0) == max_load)
            if max(self.instances + 1, self.options.min_instances) - (max_load + 1) < self.options.min_skip_instances:
                assert (max_load_dcs & dcs == max_load_dcs)
                if max_load_dcs != dcs:
                    dcs -= max_load_dcs
            hosts = [host for host in hosts if host.dc in dcs]

        return hosts

    def sort_hosts(self, hosts):
        swload = defaultdict(int)
        if self.options.swload_gran:
            for host in self.hosts:
                swload[host.switch] += 1
            for switch in swload:
                swload[switch] = swload[switch] / self.options.swload_gran

        preferred_ratio = self.options.min_instances / (self.options.min_power + 0.1)

        def smart_hosts_cmp(host1, host2):
            is_preferred1 = host1.name in self.preferred_hosts
            is_preferred2 = host2.name in self.preferred_hosts
            if is_preferred1 != is_preferred2:
                return cmp(is_preferred2, is_preferred1)
            swload1 = swload[host1.switch]
            swload2 = swload[host2.switch]
            if swload1 != swload2:
                return cmp(swload1, swload2)
            power1 = self.options.instance_power_func(host1)
            power2 = self.options.instance_power_func(host2)
            h1_ratio = len(power1) / sum(power1, .0)
            h2_ratio = len(power2) / sum(power2, .0)
            if h1_ratio != h2_ratio:
                return cmp(math.fabs(preferred_ratio - h1_ratio), math.fabs(preferred_ratio - h2_ratio))
            # remove random effect
            return cmp(host1.name, host2.name)

        hosts.sort(cmp=smart_hosts_cmp)

    def has_issues(self):
        result = False

        if self.power < self.options.min_power:
            result = True
        if self.instances < self.options.min_instances:
            result = True

        if self.power_distribution:
            result = True
        if self.instance_distribution:
            result = True

        if self.dc_power:
            skip_power = self.power - max(self.dc_power.values())
            if skip_power < self.options.min_skip_power:
                result = True

        if self.dc_instances:
            skip_instances = self.instances - max(self.dc_instances.values())
            if skip_instances < self.options.min_skip_instances:
                result = True

        return result

    def get_hosts(self):
        return copy.copy(self.hosts)

    def report(self):
        buf = StringIO()
        print >> buf, 'Hosts: %s' % len(self.hosts)
        print >> buf, 'Hosts by DC: %s' % dict([(dc, len([y for y in self.hosts if y.dc == dc]))
                                                for dc in set(x.dc for x in self.hosts)])
        print >> buf, 'Power: %s' % self.power
        print >> buf, 'Power by DC: %s' % dict(self.dc_power)
        print >> buf, 'Instances: %s' % self.instances
        print >> buf, 'Instances by DC: %s' % dict(self.dc_instances)
        swload = defaultdict(int)
        for host in self.hosts:
            swload[host.switch] += 1
        inv_swload = freq(swload.values())
        print >> buf, 'Number of switches by switch-load: %s' % inv_swload
        print >> buf, 'Selected hosts info:'
        buf.write(indent(hosts_stats(self.hosts)))
        return buf.getvalue()


def find_machines(options):
    src_hosts = calc_source_hosts(options.src_groups, [], options.host_filter,
                                  remove_slaves=not options.use_slaves, verbose=options.verbose)

    instances_by_host = {}
    for host in src_hosts:
        powers = options.instance_power_func(host)
        instances_by_host[host] = [Instance(host, n, power) for n, power in enumerate(powers)]
    instances = sum(instances_by_host.values(), [])
    src_hosts = [host for host in src_hosts if instances_by_host[host]]

    # filter hosts by group
    src_hosts = set(src_hosts)
    if options.skip_groups is not None:
        for group in options.skip_groups:
            src_hosts -= {x.host for x in group.get_kinda_busy_instances()}
    src_hosts = list(src_hosts)
    src_hosts.sort(key=lambda x: x.name)

    # filter hosts by resources
    if options.min_host_resources:
        src_hosts = [x for x in src_hosts if TResources.free_resources_skip_master(x).can_subtract(options.min_host_resources)]

    # check if we have enough power and instances
    total_power = sum([instance.power for instance in instances], .0)
    if total_power < options.min_power:
        raise Exception("Not enough power: have %f, need %f" % (total_power, options.min_power))
    total_instances = len(instances)
    if total_instances < options.min_instances:
        raise Exception("Not enough instances: have %d, need %d" % (total_instances, options.min_instances))
    if options.verbose:
        print "Total %d hosts, %d instances, %f power" % (len(src_hosts), total_instances, total_power)

    result = Result(options)
    while result.has_issues():
        next_hosts = result.filter_hosts(src_hosts)
        if not next_hosts:
            break
        result.sort_hosts(next_hosts)
        best_host = next_hosts[0]
        src_hosts = [host for host in src_hosts if host != best_host]
        result.add_host(best_host)

    if options.verbose:
        print '--------- Finished --------- '
        print result.report()

    if result.has_issues():
        raise Exception('Could not find suitable hosts.')

    if options.verbose:
        print "Chosen %d hosts with %f power:" % (len(result.hosts), sum(x.power for x in result.hosts))
        for machine in sorted(result.hosts):
            print '    Host {} free resources: {}'.format(machine.name, TResources.free_resources_skip_master(host))
        print ",".join(x.name for x in result.hosts)
    else:
        print ",".join(x.name for x in result.hosts)

    if options.move_to_group:
        if (not options.confirm) or \
                (options.confirm and prompt("Moved specified hosts to group <%s>?" % options.move_to_group.name)):
            if options.move_to_group.card.master is not None:  # slave
                if options.src_groups[0] != options.move_to_group.card.master:  # allocated hosts in master and add to slave
                    CURDB.groups.move_hosts(result.hosts, options.move_to_group.card.master)
                CURDB.groups.add_slave_hosts(result.hosts, options.move_to_group)
            else:
                CURDB.groups.move_hosts(result.hosts, options.move_to_group)

            CURDB.update(smart=True)

    return result.hosts


if __name__ == '__main__':
    options = get_parser().parse_cmd()

    normalize(options)

    machines = find_machines(options)
