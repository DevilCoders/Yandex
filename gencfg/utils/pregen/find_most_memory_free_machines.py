#!/skynet/python/bin/python

from copy import copy

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import gencfg  # noqa
from core.db import CURDB  # noqa
from core.argparse.parser import ArgumentParserExt  # noqa
import core.argparse.types as argparse_types  # noqa
from core.exceptions import UtilRuntimeException  # noqa
from core.card.types import ByteSize  # noqa
from gaux.aux_utils import correct_pfname  # noqa


RESOURCE_TYPES = ('cpu', 'memory', 'hdd', 'ssd', 'net')
RESOURCE_TYPES_UNITS = dict(cpu='Pu', memory='Gb', hdd='Gb', ssd='Gb', net='Mbit', cpu_idle='Pu')


# todo: merge duplicate with yp/resources.py
class Resources(object):
    def __init__(self, power=0, mem=0, ssd=0, hdd=0, net=0):
        self.power, self.mem, self.ssd, self.hdd, self.net = int(power), float(mem), float(ssd), float(hdd), int(net)

    def __repr__(self):
        return 'Resources(power={}, mem={}, ssd={}, hdd={}, net={})'.format(
                self.power, self.mem, self.ssd, self.hdd, self.net)

    def __getitem__(self, item):
        return {
            'cpu': self.power,
            'memory': self.mem,
            'hdd': self.hdd,
            'ssd': self.ssd,
            'net': self.net
        }[item]

    def __iadd__(self, other):
        self.power += other.power
        self.mem += other.mem
        self.ssd += other.ssd
        self.hdd += other.hdd
        self.net += other.net
        return self

    def __isub__(self, other):
        self.power -= other.power
        self.mem -= other.mem
        self.ssd -= other.ssd
        self.hdd -= other.hdd
        self.net -= other.net
        return self

    def __iter__(self):
        yield self.power
        yield self.mem
        yield self.ssd
        yield self.hdd
        yield self.net


class InstanceAssignedResources(object):
    def __init__(self, instance, group):
        self.instance = instance
        self.resources = Resources(instance.power,
                                   group.card.reqs.instances.memory_guarantee.gigabytes(),
                                   group.card.reqs.instances.ssd.gigabytes(),
                                   group.card.reqs.instances.disk.gigabytes(),
                                   group.card.reqs.instances.net_guarantee.megabits())
        if group.card.reqs.instances.get('cpu_policy') == 'idle':
            self.resources.power = 0
        else:
            self.resources.power = instance.power

    def show(self, resource_types):
        result = 'Instance {} (group {}):'.format(self.instance.name(), self.instance.type)
        for resource_type in resource_types:
            result += ' {} {:.2f} {},'.format(resource_type, self.resources[resource_type],
                                              RESOURCE_TYPES_UNITS[resource_type])
        return result

    def __str__(self):
        return 'Instance {} (group {}): memory {:.2f} Gb, cpu {:.2f} Pu, hdd {:.2f} Gb, ssd {:.2f} Gb, net {:.2f} Mb'\
            .format(self.instance.name(), self.instance.type, self.resources.mem, self.resources.power,
                    self.resources.hdd, self.resources.ssd, self.resources.net)


class HostInfo(object):
    def __init__(self, host):
        self.host = host
        self.resources = Resources(host.power, host.get_avail_memory() / 1024. ** 3,
                                   host.ssd, host.disk, host.net)
        self.host_resources = copy(self.resources)
        self.assigned = []

    def append_instance(self, instance):
        group = CURDB.groups.get_group(instance.type)

        assigned_resources = InstanceAssignedResources(instance, group)
        self.resources -= assigned_resources.resources
        self.assigned.append(assigned_resources)

    def show(self, verbose, resource_types=RESOURCE_TYPES, order='memory'):
        result = ""

        unknown_resource_types = set(resource_types) - set(RESOURCE_TYPES)
        if unknown_resource_types:
            raise Exception('Unknown resource types <{}>'.format(','.join(resource_types)))

        resource_types = sorted(set(resource_types), key=lambda x: RESOURCE_TYPES.index(x))

        not_enough_resources = any(r < 0 for r in self.resources)
        if verbose >= 1 or not_enough_resources:
            result += 'Host {} : '.format(self.host.name)
            for resource_type in resource_types:
                unit = RESOURCE_TYPES_UNITS[resource_type]
                result += ' {:.2f} {} of {:.2f} {} {} left,'.format(self.resources[resource_type], unit,
                                                                    self.host_resources[resource_type], unit,
                                                                    resource_type)
            result += '\n'
            if verbose >= 2 or not_enough_resources:
                for instance_resources in sorted(self.assigned, key=build_key_function(order), reverse=True):
                    result += '    {}\n'.format(instance_resources.show(resource_types=resource_types))

        return result.strip()

    def fits(self, requirements):
        return is_enough_resources(self, requirements)


ACTIONS = ["alloc", "show"]


class EUsedInstances(object):
    ALLOCATED = "allocated"  # instances allocated to group
    ASSIGNED = "assigned"  # busy instances in groups with intlookups, allocated instances for other groups
    INTLOOKUPS = "intlookups"  # instances in groups intlookups
    ALL = [ALLOCATED, ASSIGNED, INTLOOKUPS]


def get_parser():
    parser = ArgumentParserExt(
        description="Find machines with most quantity of free resources")
    parser.add_argument("-a", "--action", type=str, required=True,
                        choices=ACTIONS,
                        help="Obligatory. Action to execute")
    parser.add_argument("-g", "--group", type=argparse_types.group, default=None,
                        help="Obligatory. Group to search machines in")
    parser.add_argument("-s", "--hosts", type=argparse_types.hosts, default=None,
                        help="Optional. List of hosts to calculate")
    parser.add_argument("-t", "--template-group", type=argparse_types.group, required=False,
                        help="Optional. Template group to get slots per host and slot size")
    parser.add_argument("-x", "--exclude-groups", type=argparse_types.groups, default=[],
                        help="Optional. List of groups to exclude from calculating total memory")
    parser.add_argument("--slot-size", type=float, default=0,
                        help="Obligatory. Slot size of new group (when not specifying template)")
    parser.add_argument("--slot-hdd-size", type=float, default=0,
                        help="Obligatory. Slot hdd size of new group (when not specifying template)")
    parser.add_argument("--slot-ssd-size", type=float, default=0,
                        help="Obligatory. Slot ssd size of new group (when not specifying template)")
    parser.add_argument("--slot-power", type=float, default=0,
                        help="Optional. Slot power")
    parser.add_argument("--slots-per-host", type=int, default=1,
                        help="Optional. Slots per host (when not specifying template)")
    parser.add_argument("--num-slots", type=argparse_types.primus_int_count, required=False,
                        help="Obligatory. Number of required slots")
    parser.add_argument("--ignore-not-enough", action="store_true", default=False,
                        help="Optional. Ignore error when not enough slots (just add all found)")
    parser.add_argument("--add-hosts-to-group", type=argparse_types.group, default=None,
                        help="Optional. Add allocated hosts to specified group")
    parser.add_argument("--guest-group-host-size", type=float, default=None,
                        help="Optional. Create guest group hosts with specified memory size")
    parser.add_argument("-f", "--flt", type=argparse_types.pythonlambda, default=lambda x: True,
                        help="Optional. Extra filter on processed hosts")
    parser.add_argument("--used-instances", type=str, default=EUsedInstances.ASSIGNED,
                        choices=EUsedInstances.ALL,
                        help="Optional. What instances we consider as used in group")
    parser.add_argument('--resource-types', type=argparse_types.comma_list, default=RESOURCE_TYPES,
                        help='Optional. Comma-separated list of resource types to show: one or more from {}'.format(
                            ','.join(RESOURCE_TYPES)))
    parser.add_argument('--order', type=argparse_types.comma_list, default='memory',
                        help='Optional. {}. Default is memory'.format(','.join(RESOURCE_TYPES)))
    parser.add_argument("-v", "--verbose", action="count", default=0,
                        help="Optional. Verbose mode. Multiple -v options increase the verbosity. The maximum is 2.")
    parser.add_argument("-y", "--apply", action="store_true", default=False,
                        help="Optional. Apply changes")

    return parser


def normalize(options):
    if options.group and options.group.card.name == 'VLA_YT_RTC':
        forbidden_groups = [CURDB.groups.get_group(group) for group in ['GENCFG_4155']]
        options.exclude_groups.extend(forbidden_groups)
        print 'WARNING! exclude list automatically extended with', forbidden_groups, \
              'since you are allocating in VLA_YT_RTC'

    if options.template_group is not None:
        if options.group is None:
            options.group = options.template_group.card.master
        if options.group is None:
            raise Exception("You did not specify <--group> option, and template group <%s> is not a slave group".format(
                            options.template_group.card.name))

        if options.template_group.card.reqs.instances.memory_guarantee.value > 0:
            options.slot_size = options.template_group.card.reqs.instances.memory_guarantee.value / float(2 ** 30)
        else:
            raise Exception("Template group <%s> has zero memory requirements" % options.template_group.name)

        if options.template_group.card.reqs.instances.disk.value > 0:
            options.slot_hdd_size = options.template_group.card.reqs.instances.disk.value / float(2 ** 30)
        else:
            raise Exception("Template group <%s> has zero disk requirements" % options.template_group.name)

        if options.template_group.card.reqs.instances.ssd.value > 0:
            options.slot_ssd_size = options.template_group.card.reqs.instances.ssd.value / float(2 ** 30)

        ifunc_name = options.template_group.card.legacy.funcs.instanceCount
        if ifunc_name.startswith('exactly'):
            options.slots_per_host = int(options.template_group.card.legacy.funcs.instanceCount[7:])
        elif ifunc_name == 'default':
            options.slots_per_host = 1
        else:
            raise Exception("Do not know how to process ifunc <%s> from template group <%s>".format(
                            ifunc_name, options.template_group.card.name))

        options.exclude_groups.append(options.template_group)

        if options.add_hosts_to_group is None:
            options.add_hosts_to_group = options.template_group

    if options.hosts is not None:
        if options.action != "show":
            raise Exception("Only action <show> is allowed when specifying host list")
        options.verbose = 2

    if options.action != "alloc" and options.add_hosts_to_group is not None:
        raise Exception("Option --add-hosts-to-group may be used only in alloc action")

    unknown_resource_types = set(options.resource_types) - set(RESOURCE_TYPES)
    if unknown_resource_types:
        raise Exception('Unknown resource types <{}> in option <--resource-types>'.format(
            ','.join(sorted(unknown_resource_types))))

    options.resource_types = sorted(set(options.resource_types))

    if options.num_slots is not None:
        options.num_slots = int(options.num_slots)


def get_busy_instances(hosts, used_instances):
    affected_groups = set()
    for host in hosts:
        affected_groups |= set(CURDB.groups.get_host_groups(host))
    busy_instances = set()
    for group in affected_groups:
        if used_instances == EUsedInstances.ALLOCATED:
            busy_instances = set(group.get_instances())
        elif used_instances == EUsedInstances.INTLOOKUPS:
            busy_instances |= set(group.get_busy_instances())
        elif used_instances == EUsedInstances.ASSIGNED:
            busy_instances |= set(group.get_kinda_busy_instances())
    return busy_instances


def get_host_infos(options):
    if len(options.exclude_groups) > 0:
        excluded_hosts = set(sum(map(lambda x: x.getHosts(), options.exclude_groups), []))
    else:
        excluded_hosts = set()
    if options.add_hosts_to_group is not None:
        excluded_hosts |= set(options.add_hosts_to_group.getHosts())
    elif options.template_group is not None:
        excluded_hosts |= set(options.template_group.getHosts())

    if options.hosts is not None:
        filtered_hosts = set(options.hosts) - excluded_hosts
    else:
        filtered_hosts = set(filter(options.flt, options.group.getHosts())) - excluded_hosts

    return {host: HostInfo(host) for host in filtered_hosts}


def build_key_function(order):
    def fn(host):
        key = []
        for resource in order:
            if resource == 'memory':
                key.append(host.resources.mem)
            elif resource == 'hdd':
                key.append(host.resources.hdd)
            elif resource == 'ssd':
                key.append(host.resources.ssd)
            elif resource == 'net':
                key.append(host.resources.net)
            elif resource == 'cpu':
                key.append(host.resources.power)
        return tuple(key)
    return fn


def is_enough_resources(host, requirements):
    return (
        host.resources.mem >= requirements.mem and
        host.resources.hdd >= requirements.hdd and
        host.resources.ssd >= requirements.ssd and
        host.resources.power >= requirements.power
    )


def main(options):
    hosts = options.hosts if options.hosts is not None else options.group.getHosts()
    print 'SOURCE:', len(hosts), 'hosts'

    busy_instances = get_busy_instances(hosts, options.used_instances)  # all instances from affected groups, not filtered by hosts and flt
    all_used_instances = []
    for host in hosts:
        groups = CURDB.groups.get_host_groups(host)
        host_instances = set(sum((group.get_host_instances(host) for group in groups), []))
        host_instances &= busy_instances
        all_used_instances.extend(list(host_instances))
    filtered_used_instances = filter(lambda instance: options.flt(instance.host), all_used_instances)  # instances from affected groups, filtered by hosts and flt
    hosts_info = get_host_infos(options)  # filtered by -x groups, not filtered by flt

    for instance in filtered_used_instances:
        if instance.host in hosts_info:
            hosts_info[instance.host].append_instance(instance)
    hosts_info = sorted(hosts_info.values(), key=build_key_function(options.order), reverse=True)

    if options.action == 'alloc':
        options.num_slots = max(0, options.num_slots)

        reqs = Resources(
            mem=options.slot_size * options.slots_per_host,
            hdd=options.slot_hdd_size * options.slots_per_host,
            ssd=options.slot_ssd_size * options.slots_per_host,
            power=options.slot_power * options.slots_per_host
        )

        if options.verbose >= 2:
            print "Unsuitable hosts"
            print "==============================================================================================="
            for elem in filter(lambda x: not is_enough_resources(x, reqs), hosts_info):
                print elem.show(options.verbose, resource_types=options.resource_types)
            print "==============================================================================================="

        hosts_info = filter(lambda x: is_enough_resources(x, reqs), hosts_info)
        needed_slots = options.num_slots / options.slots_per_host + ((options.num_slots % options.slots_per_host) > 0)

        if len(hosts_info) < needed_slots:
            if options.ignore_not_enough:
                if options.verbose >= 1:
                    print 'Found only <{}> slots when requesting <{}> (allocating all found)'.format(
                        len(hosts_info), needed_slots)
                needed_slots = len(hosts_info)
            else:
                raise UtilRuntimeException(correct_pfname(__file__),
                                           "Not enough hosts: have %d, needed %d" % (len(hosts_info), needed_slots))

        if (options.add_hosts_to_group is not None) and options.apply:
            if options.guest_group_host_size is not None:  # FIXME: hack
                old_size = options.add_hosts_to_group.card.reqs.instances.memory_guarantee
                new_size = ByteSize('{} Gb'.format(options.guest_group_host_size))
                try:
                    options.add_hosts_to_group.card.reqs.instances.memory_guarantee = new_size
                    CURDB.groups.add_slave_hosts(map(lambda x: x.host, hosts_info[:needed_slots]),
                                                 options.add_hosts_to_group)
                finally:
                    options.add_hosts_to_group.card.reqs.instances.memory_guarantee = old_size
            else:
                if options.add_hosts_to_group.card.master is None:  # add hosts to master background groups
                    CURDB.groups.add_hosts(map(lambda x: x.host, hosts_info[:needed_slots]), options.add_hosts_to_group)
                else:
                    CURDB.groups.add_slave_hosts(map(lambda x: x.host, hosts_info[:needed_slots]),
                                                 options.add_hosts_to_group)

            options.add_hosts_to_group.mark_as_modified()
            CURDB.update(smart=True)

        if options.verbose >= 1:
            print "Allocated %d slots" % needed_slots

        return hosts_info[:needed_slots]
    elif options.action == 'show':
        return hosts_info
    else:
        raise Exception("Unknown action <%s>" % options.action)


def print_result(result, options):
    if options.action == "alloc":
        if options.verbose == 0:
            print "Allocated %d hosts (showing first 100): %s" % \
                  (len(result), ",".join(map(lambda x: x.host.name, result[:100])))
        else:
            for host_info in result:
                host_result = host_info.show(options.verbose, resource_types=options.resource_types, order=options.order)
                if host_result:
                    print host_result
    elif options.action == "show":
        for host_info in result:
            host_result = host_info.show(options.verbose, resource_types=options.resource_types, order=options.order)
            if host_result:
                print host_result
    else:
        raise Exception("Unknown action <%s>" % options.action)


def jsmain(d):
    options = get_parser().parse_json(d)
    normalize(options)
    return main(options)


def cli_main():
    options = get_parser().parse_cmd()
    normalize(options)
    result = main(options)
    print_result(result, options)


if __name__ == '__main__':
    cli_main()
