#!/skynet/python/bin/python
"""
    In this script we perfom same recluster actions for non-dynamic group. The following actions are supported so far:
        - change memory slot size for slave group without host donor and zero power requirements;
        - change number of instances in group;
"""

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

from collections import defaultdict

import gencfg
from core.db import CURDB
import utils.pregen.find_most_memory_free_machines as find_most_memory_free_machines
from core.argparse.parser import ArgumentParserExt
import core.argparse.types as argparse_types
from core.exceptions import UtilNormalizeException
from gaux.aux_utils import correct_pfname
from gaux.aux_colortext import red_text


class TReport(object):
    """
        Class, representing changes report
    """

    def __init__(self, group, new_memory_size, extra_instances_count):
        """
            Init with group and other parameters

            :param group(core.igroups.IGroups): group to process
            :param new_memory_size(core.card.types.ByteSize): new memory size
        """

        self.group = group
        self.old_memory_size = self.group.card.reqs.instances.memory_guarantee.value / 1024. / 1024 / 1024
        self.new_memory_size = new_memory_size.value / 1024. / 1024 / 1024

        self.old_instances_count = self.group.card.reqs.instances.min_instances
        self.new_instances_count = self.old_instances_count + extra_instances_count

        self.replacements = []  # pairs of host for replacement

    def report_text(self):
        result = ["Group %s:" % self.group.card.name,
                  "    Change memory limit from <%.3f Gb> to <%.3f Gb> (%d replacements):" % (self.old_memory_size, self.new_memory_size, len(self.replacements)),
                  "    Change instance count by <%d> instances:" % (self.new_instances_count - self.old_instances_count)]
        for oldhost, newhost in self.replacements:
            result.append("        %s -> %s" % (oldhost.name if oldhost else None, newhost.name if newhost else None))

        return "\n".join(result)

    def apply(self):
        """
            Apply changes
        """
        self.group.card.reqs.instances.memory_guarantee.reinit("%.3f Gb" % self.new_memory_size)
        self.group.card.reqs.instances.min_instances = self.new_instances_count

        for oldhost, newhost in self.replacements:
            if oldhost is None:
                self.group.addHost(newhost)
            elif newhost is None:
                self.group.removeHost(oldhost)
            else:
                self.group.replace_host(oldhost, newhost)

    def rollback(self):
        """
            Rollback changes
        """
        self.group.card.reqs.instances.memory_guarantee.reinit("%.3f Gb" % self.old_memory_size)
        self.group.card.reqs.instances.min_instances = self.old_instances_count

        for oldhost, newhost in self.replacements:
            self.group.replace_host(newhost, oldhost)


def get_parser():
    parser = ArgumentParserExt(description="Some recluster actions for web groups")
    parser.add_argument("-g", "--group", type=argparse_types.group, required=True,
                        help="Obligatory. Group to perform action on")
    parser.add_argument("--memory", type=argparse_types.byte_size, default=None,
                        help="Optional. New memory size as str, e. g. '12 Gb'")
    parser.add_argument("--instances", dest="extra_instances", type=int, default=0,
                        help="Optional. Change number of instances by specified amount (negative means remove some instances). Power of every instance will be average of all current instances.")
    parser.add_argument("-y", "--apply", action="store_true", default=False,
                        help="Optional. Apply changes")

    return parser


def normalize(options):
    # check if we can recluster group of this type
    if options.group.card.properties.fake_group:
        raise UtilNormalizeException(correct_pfname(__file__), ["group"],
                                     "Group <%s> is fake group (recluster of such groups is not yet supported)" % options.group.card.name)
    if options.group.card.properties.full_host_group:
        raise UtilNormalizeException(correct_pfname(__file__), ["group"],
                                     "Group <%s> is full host group (recluster of such groups is not yet supported)" % options.group.card.name)
    if options.group.card.host_donor is not None:
        raise UtilNormalizeException(correct_pfname(__file__), ["group"],
                                     "Group <%s> has host donor <%s> (recluster of such groups is not yet supported)" %
                                     (options.group.card.name, options.group.card.host_donor))
    if options.group.card.master is None:
        raise UtilNormalizeException(correct_pfname(__file__), ["group"],
                                     "Group <%s> is master group (recluster of such groups is not yet supported)" % options.group.card.name)

    # check for params ranges
    if (options.group.card.reqs.instances.min_instances == 0) and (options.extra_instances != 0):
        raise UtilNormalizeException(correct_pfname(__file__), ["group"],
                                     "For group <%s> minimal number of instances is not set in group card" % options.group.card.name)
    if options.group.card.reqs.instances.min_instances + options.extra_instances < 0:
        raise UtilNormalizeException(correct_pfname(__file__), ["group"],
                                     "Group <%s> has <%d> instances while requested decreasing by <%s>" % (options.group.card.name, options.group.card.reqs.instances.min_instances, -options.extra_instances))

    # fill some parameters
    ifunc_name = options.group.card.legacy.funcs.instanceCount
    if ifunc_name.startswith('exactly'):
        options.slots_per_host = int(ifunc_name[7:])
    elif ifunc_name == 'default':
        options.slots_per_host = 1
    else:
        # do not know how many slots per host we have, so suppose current maximum per host is correct value
        instances_by_host = defaultdict(int)
        for instance in options.group.get_kinda_busy_instances():
            instances_by_host[instance.host] += 1
        options.slots_per_host = max(instances_by_host.values())
    if options.memory is None:
        options.memory = options.group.card.reqs.instances.memory_guarantee


def run_change_slot_size(report, options, from_cmd=True):
    options.group.mark_as_modified()

    if options.group.card.reqs.instances.memory_guarantee.value > options.memory.value:
        pass
    else:
        # first find machines with not enough memory
        util_params = {
            "action": "show",
            "group": options.group.card.master,
        }
        failed_hosts_result = find_most_memory_free_machines.jsmain(util_params)

        print "Step0 failed hosts: %s" % len(failed_hosts_result)

        group_hosts = set(options.group.getHosts())
        failed_hosts_result = filter(lambda x: x.host in group_hosts, failed_hosts_result)

        print "Step1 failed hosts: %s" % len(failed_hosts_result)

        needed_memory = (
                        options.memory.value - options.group.card.reqs.instances.memory_guarantee.value) / 1024. / 1024. / 1024.

        failed_hosts_result = filter(lambda x: x.memory_left < needed_memory, failed_hosts_result)

        remove_hosts = map(lambda x: x.host, failed_hosts_result)

        # than find replacement
        util_params = {
            "action": "alloc",
            "group": options.group.card.master,
            "exclude_groups": [options.group],
            "slot_size": options.memory.gigabytes(),
            "slots_per_host": options.slots_per_host,
            "num_slots": str(len(failed_hosts_result)),
        }

        replacement_hosts_result = find_most_memory_free_machines.jsmain(util_params)

        add_hosts = map(lambda x: x.host, replacement_hosts_result)

        report.replacements.extend(zip(remove_hosts, add_hosts))

    return report

def run_change_instances_count(report, options, from_cmd=True):
    if options.extra_instances < 0:
        # FIXME: remove non-arbitrary instances
        to_remove = -options.extra_instances
        for host in options.group.getHosts():
            host_instances = options.group.get_host_instances(host)
            if len(host_instances) > to_remove:
                to_remove = 0
                break
            else:
                report.replacements.append((host, None))
                to_remove -= len(host_instances)
        if to_remove != 0:
            raise Exception, "Can not remove <%d> instances in group <%s>" % (to_remove, options.group.card.name)
    else:
        # find extra instances using find_most_memory_free_machines
        util_options = {
            "action": "alloc",
            "group": options.group.card.master,
            "template_group": options.group,
            "num_slots": options.extra_instances,
        }
        hosts_to_add = find_most_memory_free_machines.jsmain(util_options)
        for host in hosts_to_add:
            report.replacements.append((None, host.host))

    return report

def main(options, from_cmd=True):
    report = TReport(options.group, options.memory, options.extra_instances)

    run_change_slot_size(report, options, from_cmd=from_cmd)
    run_change_instances_count(report, options, from_cmd=from_cmd)

    if options.apply:
        report.apply()
        CURDB.update(smart=True)
    else:
        if not from_cmd:
            print red_text("Not updated!!! Add option -y to update.")

    return report


def print_result(result):
    print result.report_text()


if __name__ == '__main__':
    options = get_parser().parse_cmd()

    normalize(options)

    result = main(options, from_cmd=True)

    print_result(result)
