#!/skynet/python/bin/python
"""
    Script to perform more complex actions, than basic allocation in dynamic. It is based on allocation in dynamic, so main part is to prepare
    input data and invoke main allocation function. Currently supported actions:
        - CHANGE SLOT SIZE: we can not increase slot size in dynamic, but we often want to. In this script we check, which hosts have enough free memory, which have not. Then we replace hosts with not enough memory by invocation of main optimization function.
        - CHANGE NUMBER OF INSTANCES: we can add/remove instances from group (power of remaining instances remain unchanged in case of deletion, new instances created with avg power in case of adding)
        - CHANGE GROUP POWER: we can change group powe by changing instances power or adding extra ones if do not have enough power on current hosts

    All actions can be performed together.
"""

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import re
from collections import namedtuple, defaultdict

import gencfg
from core.db import CURDB
from core.argparse.parser import ArgumentParserExt
import core.argparse.types as argparse_types
from core.exceptions import UtilNormalizeException
from gaux.aux_utils import correct_pfname, get_instance_used_ports, get_group_used_ports
from core.card.types import ByteSize
from gaux.aux_colortext import red_text
from core.igroups import CIPEntry
import utils.common.find_most_unused_port as find_most_unused_port

from aux_dynamic import calculate_free_resources

from optimizers.dynamic.main import HostRoom


TResourceTypeInfo = namedtuple('TResourceTypeInfo', ['name', 'group_req_func', 'host_func', 'instance_func'])


RESOURCE_TYPES = dict(
    # memory
    memory = TResourceTypeInfo(
                name='memory',
                group_req_func=lambda x: x.card.reqs.instances.memory_guarantee,
                host_func=lambda x: x.get_avail_memory(),
                instance_func=lambda x: CURDB.groups.get_group(x.type).card.reqs.instances.memory_guarantee.value,
    ),
    # hdd
    hdd = TResourceTypeInfo(
            name='hdd',
            group_req_func=lambda x: x.card.reqs.instances.disk,
            host_func=lambda x: x.disk * 1024 * 1024 * 1024,
            instance_func=lambda x: CURDB.groups.get_group(x.type).card.reqs.instances.disk.value,
    ),
    # ssd
    ssd = TResourceTypeInfo(
            name='ssd',
            group_req_func=lambda x: x.card.reqs.instances.ssd,
            host_func=lambda x: x.ssd * 1024 * 1024 * 1024,
            instance_func=lambda x: CURDB.groups.get_group(x.type).card.reqs.instances.ssd.value,
    ),
    # net
    net = TResourceTypeInfo(
            name='net',
            group_req_func=lambda x: x.card.reqs.instances.net_guarantee,
            host_func=lambda x: x.net / 10. * 1024 * 1024,  # net stored in Mbits/sec
            instance_func=lambda x: CURDB.groups.get_group(x.type).card.reqs.instances.net_guarantee.value,
    ),
)


class TReport(object):
    """
        Class, representing changes report
    """

    class TGroupState(object):
        """
            Class with all group data (all data are cached and not changed, when anything happen with group instances/card
        """

        def __init__(self, group):
            self.groupname = group.card.name
            self.memory = group.card.reqs.instances.memory_guarantee.value

            self.instances = {}
            for instance in group.get_instances():
                self.instances[(instance.host.name, instance.port)] = instance.power

    def __init__(self):
        self.state_before = None
        self.state_after = None

    def report_text(self):
        assert (self.state_before is not None and self.state_after is not None)
        assert (self.state_before.groupname == self.state_after.groupname)

        result = []
        result.append("Group %s:" % self.state_before.groupname)
        if self.state_before.memory != self.state_after.memory:
            result.append("    Memory Slot Size: %.2f Gb -> %.2f Gb" % (
            self.state_before.memory / 1024. / 1024. / 1024., self.state_after.memory / 1024. / 1024. / 1024.))

        added_instances = set(self.state_after.instances.keys()) - set(self.state_before.instances.keys())
        removed_instances = set(self.state_before.instances.keys()) - set(self.state_after.instances.keys())
        modified_instances = set(self.state_before.instances.keys()) & set(self.state_after.instances.keys())

        unchanged_instances = filter(lambda x: self.state_before.instances[x] == self.state_after.instances[x],
                                     modified_instances)
        modified_instances = filter(lambda x: self.state_before.instances[x] != self.state_after.instances[x],
                                    modified_instances)

        result.append("    Instances changes: %s added %s removed %s modified %s unchanged" % (
        len(added_instances), len(removed_instances),
        len(modified_instances), len(unchanged_instances)))

        if len(added_instances) > 0:
            result.append("        Added:")
            for host, port in added_instances:
                result.append("            Instance %s:%s: %d power" % (
                host, port, int(self.state_after.instances[(host, port)])))

        if len(removed_instances) > 0:
            result.append("        Removed:")
            for host, port in removed_instances:
                result.append("            Instance %s:%s: %d power" % (
                host, port, int(self.state_before.instances[(host, port)])))

        if len(modified_instances) > 0:
            result.append("        Modified:")
            for host, port in modified_instances:
                result.append("            Instance:%s:%s: %d -> %d power" % (
                host, port, int(self.state_before.instances[(host, port)]),
                int(self.state_after.instances[(host, port)])))

        return "\n".join(result)


def is_free_ports(hosts, need_free_ports):
    """Check if ports from specified list are not used on specified hosts"""
    used_ports = set()
    for host in hosts:
        for instance in CURDB.groups.get_host_instances(host):
            used_ports |= get_instance_used_ports(CURDB, instance)

    intersection = used_ports & set(need_free_ports)

    return len(intersection) == 0


def commit_with_optional_change_port(group, allocation_report):
    """Add instances to group with optional change of group port"""

    extra_hosts = [x.host for x in allocation_report.allocated_resources]

    if len(group.get_instances()):
        need_free_ports = get_instance_used_ports(CURDB, group.get_instances()[0])
    else:
        need_free_ports = get_group_used_ports(group)

    if not is_free_ports(extra_hosts, need_free_ports):
        util_params = dict(strict=True, port_range=8, hosts=extra_hosts + group.getHosts())
        group.card.reqs.instances.port = find_most_unused_port.jsmain(util_params).port
        # ======================================= GENCFG-2529 START =====================================
        group.card.properties.monitoring_ports_ready = False
        group.card.properties.monitoring_golovan_port = None
        group.card.properties.monitoring_juggler_port = None
        # ======================================= GENCFG-2529 FINISH =====================================

        print 'Had to change group {} port to {}'.format(group.card.name, group.card.reqs.instances.port)

    allocation_report.commit(group)


def get_parser():
    parser = ArgumentParserExt(description="More complex actions on dynamic")
    parser.add_argument("-g", "--group", type=argparse_types.group, required=True,
                        help="Obligatory. Group to perform action on")
    parser.add_argument("--memory", type=str, default=None,
                        help="Optional. New memory size as str, e. g. '12 Gb'. Can specify in form '+1 Gb' or '-2 Gb' in order to increase/decrease memory size by specified amount")
    parser.add_argument("--power", type=int, default=None,
                        help="Optional. Change group power by speicified amount (in gencfg units, negative means decrease group power).")
    parser.add_argument("--instances", type=int, default=None,
                        help="Optional. Change number of instances by specified amount (negative means remove some instances). Power of every instance will be average of all current instances.")
    parser.add_argument("--ssd", type=str, default=None,
                        help="Optional. New required ssd size as str, e. g. '12 Gb'.")
    parser.add_argument("--hdd", type=str, default=None,
                        help="Optional. New required hdd size as str, e. g. '12 Gb'.")
    parser.add_argument("--net", type=str, default=None,
                        help="Optional. New required net size as str, e. g. '11 Mb'.")
    parser.add_argument("--exclude-groups-flt", type=argparse_types.pythonlambda, default=lambda x: False,
                        help="Optional. Lambda function with filter on groups to be excluded when selecting hosts from free list")
    parser.add_argument('--hosts-flt', type=argparse_types.pythonlambda, default=lambda x: True,
                        help="Optional. Lambda function with filter on hosts when selecting hosts from free list")
    parser.add_argument("--minimize-replicas-count", action="store_true", default=False,
                        help="Optional. Minimize replicas count")
    parser.add_argument("--simple", action="store_true", default=False,
                        help="Optional. Run simple algorithm of optimization")
    parser.add_argument("-v", "--verbose", action="count", default=0,
                        help="Optional. Verbose level (specify multiple times to increase verbosity)")
    parser.add_argument("-y", "--apply", action="store_true", default=False,
                        help="Optional. Apply changes")

    return parser


def normalize(options):
    if options.group.card.master is None:
        raise Exception('Group <{}> is not a slave group'.format(options.group.card.name))
    if options.group.card.host_donor is not None:
        raise Exception('Group <{}> has host donor, thus can not be optimized'.format(options.group.card.name))

    if options.power is not None:
        options.power = sum(map(lambda x: x.power, options.group.get_instances())) + options.power
        options.power = max(0, options.power)

    if options.instances is not None:
        options.instances += len(options.group.get_instances())
        options.instances = max(1, options.instances)

    if options.memory is not None:
        if options.memory.startswith('+'): # +1Gb means add 1 Gb to memory
            options.memory = options.group.card.reqs.instances.memory_guarantee.value + ByteSize(options.memory[1:]).value
            options.memory /= (1024. * 1024 * 1024)
            options.memory = ByteSize('%.2f Gb' % options.memory)
        elif options.memory.startswith('-'): # -1Gb means add 1 Gb to memory
            options.memory = options.group.card.reqs.instances.memory_guarantee.value - ByteSize(options.memory[1:]).value
            options.memory /= (1024. * 1024 * 1024)
            options.memory = ByteSize('%.2f Gb' % options.memory)
        else:
            options.memory = ByteSize(options.memory)

    if options.ssd is not None:
        options.ssd = ByteSize(options.ssd)

    if options.hdd is not None:
        options.hdd = ByteSize(options.hdd)

    if options.net is not None:
        options.net = ByteSize(options.net)



def run_change_resource_size(options, resource_type, from_cmd=True):
    """This function allows us to change any resource (memory/ssd/hdd/net) size (make it bigger or smaller) for specified group in dynamic."""

    group = options.group

    required = getattr(options, resource_type.name)

    if resource_type.group_req_func(group).value > required.value:
        resource_type.group_req_func(group).value = required.value
        resource_type.group_req_func(group).text = required.text
    else:
        import optimizers.dynamic.main as update_fastconf

        resource_type.group_req_func(group).value = required.value
        resource_type.group_req_func(group).text = required.text

        # check, how many hosts have enough free resources
        hosts = options.group.getHosts()
        free_resource_dict, _ = calculate_free_resources(CURDB, hosts, resource_type.host_func, resource_type.instance_func, check_zero=False)

        enough_resource_hosts = filter(lambda (host, resource_left): resource_left >= 0, free_resource_dict.iteritems())
        not_enough_resource_hosts = filter(lambda (host, resource_left): resource_left < 0, free_resource_dict.iteritems())
        not_enough_resource_hosts = map(lambda (x, y): x, not_enough_resource_hosts)

        # find new hosts to replace hosts with not enough resource
        if len(not_enough_resource_hosts) > 0:
            total_power = int(sum(map(lambda x: x.power, filter(lambda x: x.host in not_enough_resource_hosts,
                                                                options.group.get_instances()))))
            update_fastconf_options = {
                "action": "add",
                "master_group": options.group.card.master.card.name,
                "group": options.group.card.name,
                "add_to_existing": True,
                "min_replicas": len(not_enough_resource_hosts),
                "max_replicas": len(not_enough_resource_hosts),
                "min_power": total_power,
                "equal_instances_power": options.group.card.reqs.shards.equal_instances_power,
                "exclude_groups_flt": options.exclude_groups_flt,
                "hosts_flt": options.hosts_flt,
            }

            if options.minimize_replicas_count:
                update_fastconf_options["max_replicas"] = len(not_enough_resource_hosts)

            update_fastconf_group, allocation_report = update_fastconf.jsmain(update_fastconf_options, CURDB,
                                                                              from_cmd=False)

            if options.verbose:
                print 'Change slot params: {}'.format(update_fastconf_options)
                print 'Change slot result: {}'.format(str(allocation_report))

            # remove hosts with not enough resource
            CURDB.groups.remove_slave_hosts(not_enough_resource_hosts, options.group)

            commit_with_optional_change_port(update_fastconf_group, allocation_report)


def run_change_instances_power(options, from_cmd=True):
    """
        This function allows dynamic change of instances power
    """

    group = options.group
    group_power = sum(map(lambda x: x.power, group.get_instances()))

    if options.power < group_power:  # The easiest case
        if options.verbose:
            'Change instances power: reducing power from {} to {} (nothing to do)'.format(group_power, options.power)
        reduce_power = int(group_power - options.power)
        instances = group.get_instances()
        while reduce_power > 0:
            # FIXME: extremely inefficient
            instances.sort(cmp=lambda x, y: cmp(y.power, x.power))
            if instances[0].power == 1:
                break
            instances[0].power -= 1
            reduce_power -= 1

        for instance in instances:
            group.custom_instance_power[CIPEntry(instance.host, instance.port)] = instance.power
    else:
        if options.simple:
            if not group.card.reqs.shards.equal_instances_power:
                raise Exception('Group <{}> has not-equal instances power, thus simple optimization does not work'.format(group.card.name))

        instances = group.get_instances()

        power_per_instance = int(options.power) / len(instances)
        if options.power % len(instances) != 0:
            power_per_instance += 1

        # Get instances by host
        instances_by_host = defaultdict(list)
        for instance in instances:
            instances_by_host[instance.host].append(instance)

        # Find not enough hosts
        not_enough_power_hosts = set()
        not_enough_power_instances_count = 0
        for host, host_instances in instances_by_host.iteritems():
            power_per_host = power_per_instance * len(host_instances)
            used_host_power = sum(map(lambda x: x.power, host_instances))

            if HostRoom.free_room(host, group.parent.db).power < power_per_host - used_host_power:
                not_enough_power_hosts.add(host)
                not_enough_power_instances_count += len(host_instances)
            else:
                for instance in host_instances:
                    instance.power = power_per_instance
                    group.custom_instance_power[CIPEntry(instance.host, instance.port)] = power_per_instance
        not_enough_power_hosts = list(not_enough_power_hosts)

        print('Replace hosts:')
        for host in not_enough_power_hosts:
            print('    {}'.format(host.name))

        # allocate extra instances
        if len(not_enough_power_hosts):
            update_fastconf_options = {
                "action": "add",
                "master_group": group.card.master.card.name,
                "group": group.card.name,
                "add_to_existing": True,
                "min_replicas": len(not_enough_power_hosts),
                "max_replicas": len(not_enough_power_hosts),
                "min_power": power_per_instance * not_enough_power_instances_count,
                "equal_instances_power": True,
                "exclude_groups_flt": options.exclude_groups_flt,
                "hosts_flt": options.hosts_flt,
            }

            import optimizers.dynamic.main as update_fastconf
            update_fastconf_group, allocation_report = update_fastconf.jsmain(update_fastconf_options, CURDB,
                                                                              from_cmd=False)
            # remove hosts with not enough memory
            CURDB.groups.remove_slave_hosts(not_enough_power_hosts, options.group)

            if options.verbose:
                print 'Change instances power params: {}'.format(update_fastconf_options)
                print 'Change instances power result: {}'.format(str(allocation_report))

            commit_with_optional_change_port(update_fastconf_group, allocation_report)


def run_change_instances_count(options, from_cmd=True):
    group = options.group
    instances = group.get_instances()

    if options.instances < len(instances):
        if len(group.card.intlookups) == 0:
            # remove instances with least power
            instances.sort(cmp=lambda x, y: cmp(x.power, y.power))
            for instance in instances[:-options.instances]:
                group.removeHost(instance.host)
        elif len(group.card.intlookups) == 1:
            # ============================================== RX-477 START =====================================
            intlookup = CURDB.intlookups.get_intlookup(group.card.intlookups[0])
            intlookup.mark_as_modified()
            multishards = intlookup.get_multishards()
            to_remove = len(instances) - options.instances
            while to_remove > 0:
                multishards.sort(key = lambda x: len(x.brigades))
                multishard = multishards[-1]
                intgroup = multishard.brigades.pop()
                instances = intgroup.get_all_basesearchers()

                to_remove -= len(instances)
                for host in (x.host for x in instances):
                    group.removeHost(host)
            # ============================================== RX-477 FINISH ====================================
        else:
            raise Exception('Removing instances from group with more than one intlookup is not implemented')
    else:
        # allocate instances with avg power
        to_add = options.instances - len(instances)

        if len(instances) > 0:
            avg_power = int(sum(map(lambda x: x.power, instances)) / len(instances))
        else:
            m = re.search('exactly(\d+)', group.card.legacy.funcs.instancePower)
            if m:
                avg_power = int(m.group(1))
            else:
                avg_power = group.card.reqs.shards.min_power or 1
        avg_power = max(1, avg_power)
        needed_power = avg_power * to_add

        update_fastconf_options = {
            "action": "add",
            "master_group": group.card.master.card.name,
            "group": group.card.name,
            "add_to_existing": True,
            "min_replicas": to_add,
            "max_replicas": to_add,
            "min_power": needed_power,
            "equal_instances_power": group.card.reqs.shards.equal_instances_power,
            "exclude_groups_flt": options.exclude_groups_flt,
            "hosts_flt": options.hosts_flt,
        }

        import optimizers.dynamic.main as update_fastconf
        update_fastconf_group, allocation_report = update_fastconf.jsmain(update_fastconf_options, CURDB,
                                                                          from_cmd=False)

        if options.verbose:
            print 'Change instances count params: {}'.format(update_fastconf_options)
            print 'Change instances count result: {}'.format(str(allocation_report))

        commit_with_optional_change_port(update_fastconf_group, allocation_report)


def main(options, from_cmd=True):
    report = TReport()
    report.state_before = TReport.TGroupState(options.group)

    options.group.custom_instance_power_used = True
    options.group.mark_as_modified()

    if options.ssd is not None:
        run_change_resource_size(options, RESOURCE_TYPES['ssd'], from_cmd=from_cmd)
    if options.hdd is not None:
        run_change_resource_size(options, RESOURCE_TYPES['hdd'], from_cmd=from_cmd)
    if options.net is not None:
        run_change_resource_size(options, RESOURCE_TYPES['net'], from_cmd=from_cmd)
    if options.memory is not None:
        run_change_resource_size(options, RESOURCE_TYPES['memory'], from_cmd=from_cmd)
    if options.power is not None:
        run_change_instances_power(options, from_cmd=from_cmd)
    if options.instances is not None:
        run_change_instances_count(options, from_cmd=from_cmd)

    if options.apply:
        CURDB.update(smart=True)
    elif not from_cmd:
        print red_text("Not updated!!! Add option -y to update.")

    report.state_after = TReport.TGroupState(options.group)

    return report


def print_result(result):
    print result.report_text()


def jsmain(d):
    options = get_parser().parse_json(d)
    normalize(options)
    return main(options)


if __name__ == '__main__':
    options = get_parser().parse_cmd()

    normalize(options)

    result = main(options, from_cmd=True)

    print_result(result)
