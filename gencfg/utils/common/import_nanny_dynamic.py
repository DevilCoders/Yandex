#!/skynet/python/bin/python
"""
    Script to import nanny dynamic services into gencfg. Task https://st.yandex-team.ru/GENCFG-291
"""

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import re
import math
import urllib2
from collections import defaultdict

import gencfg
from core.db import CURDB
from core.argparse.parser import ArgumentParserExt
import core.argparse.types
from core.card.types import ByteSize
from core.exceptions import UtilRuntimeException
from gaux.aux_utils import load_nanny_url
from gaux.aux_staff import get_possible_group_owners
from gaux.aux_colortext import red_text


class CanNotBeImportedException(Exception): pass


class TNannyAllocation(object):
    def __init__(self, allocation_info, cluster, allow_owners, options):
        """
            Constructor for intermediate class when importing data from nanny to gencfg.

            :param allocation_info(dict): jsoned data for single allocation (https://nanny.yandex-team.ru/ui/#/resourcesmanager/api-docs)
            :param cluster(str): cluster identifier (msk/sas/man/test/???)
        """

        self.master = options.group
        self.name = self._gen_group_name(allocation_info, cluster)
        self.allocation_id = allocation_info["id"][:6].upper()

        host_resources = allocation_info["host_resources"]
        for resource_name in ["tcp_ports", "memory", "disk"]:
            if resource_name == "tcp_ports":
                resource_values = list(set(map(lambda x: tuple(x["resources"][resource_name][0]), host_resources)))
                if len(resource_values) > 1:
                    # print red_text("Allocation <%s, %s> has different %s: <%s> (thus will be imported with random ports)"  % (
                    #    allocation_info["id"], self.name, resource_name, resource_values))
                    resource_values = [[None]]
                else:
                    resource_values = [resource_values[0]]
            else:
                resource_values = list(set(map(lambda x: x["resources"][resource_name], host_resources)))

            if len(resource_values) > 1:
                raise CanNotBeImportedException(
                    "Allocation <%s, %s> has different %s: <%s> (thus can not be imported)" % (
                        allocation_info["id"], self.name, resource_name, resource_values))
            setattr(self, resource_name, resource_values[0])
        self.port = self.tcp_ports[0]

        # filling power a bit more tricky
        self.hosts_data = {}
        for elem in host_resources:
            if CURDB.hosts.has_host(elem["host_id"]):
                hostname = elem["host_id"]
            else:
                hostname = CURDB.hosts.resolve_short_name(elem["host_id"].partition(".")[0])
            host = CURDB.hosts.get_host_by_name(hostname)

            assert (options.group.hasHost(host)), "Host <%s> does not belong to dynamic nanny group <%s>" % (
            host.name, options.group.card.name)

            model = CURDB.cpumodels.get_model(host.model)
            self.hosts_data[host] = elem["resources"]["cpu"] / 100. / model.ncpu * model.power

        # load group owners from nanny services
        try:
            candidate_owners = sorted(list(set(
                load_nanny_url("v2/services/%s/auth_attrs/" % allocation_info["role"].rpartition("/")[2])["content"][
                    "owners"]["logins"])))
            self.owners = filter(lambda x: x in allow_owners, candidate_owners)
            if len(self.owners) == 0:
                raise CanNotBeImportedException(
                    "Allocation <%s, %s> has only incorrect owners <%s>" % (allocation_info["id"], self.name, ",".join(candidate_owners)))
        except UtilRuntimeException:
            raise CanNotBeImportedException(
                "Allocation <%s, %s> can not load group owners" % (allocation_info["id"], self.name))
        except Exception, e:
            raise CanNotBeImportedException(
                "Allocation <%s, %s> can not load group owners due to exception <%s>" % (allocation_info["id"], self.name, str(e)))

    def _gen_group_name(self, allocation_info, cluster):
        """
            Make human-readable name from allocation
        """

        groupname = ("%s_%s" % (cluster, allocation_info["role"].rpartition("/")[2])).upper()
        groupname = re.sub("[^A-Z0-9_]", "_", groupname)

        return groupname

    def is_equal_to_gencfg(self):
        gencfg_group = CURDB.groups.get_group(self.name)

        if int(gencfg_group.card.reqs.instances.disk.gigabytes()) != self.disk:
            return False
        if int(gencfg_group.card.reqs.instances.memory_guarantee.megabytes()) != self.memory:
            return False
        if gencfg_group.card.reqs.instances.port != self.port:
            return False
        if set(gencfg_group.card.owners) != set(self.owners):
            return False

        for instance in gencfg_group.get_instances():
            if instance.host not in self.hosts_data:
                return False
            if math.fabs(instance.power - self.hosts_data[instance.host]) > 0.1:
                return False

        return True

    def import_to_gencfg(self):
        """
            Import group to gencfg. Suppose, group does not exists or removed by this point

            :return (core.igroups.IGroup): gencfg group
        """

        assert (not CURDB.groups.has_group(self.name)), "Group <%s> already exists" % self.name

        group = CURDB.groups.add_group(self.name, description="Automatically loaded from nanny",
                                       instance_port_func="old%s" % self.port,
                                       master=self.master.card.name)
        group.card.reqs.instances.disk = ByteSize("%s Gb" % self.disk)
        group.card.reqs.instances.memory_guarantee = ByteSize("%s Mb" % self.memory)
        group.card.reqs.instances.port = self.port
        group.custom_instance_power_used = True
        group.card.owners = self.owners
        group.card.tags.metaprj = 'nanny-dynamic'

        for host in self.hosts_data:
            group.addHost(host)
        for instance in group.get_instances():
            instance.power = self.hosts_data[instance.host]


def get_parser():
    parser = ArgumentParserExt("Import nanny services into gencfg")

    parser.add_argument("-g", "--group", type=core.argparse.types.group, required=True,
                        help="Obligatory. Container for dynamic nanny groups")
    parser.add_argument("-v", "--verbose", action="count", default=0,
                        help="Optional. Increase output verbosity (maximum 2)")
    parser.add_argument("-y", "--apply", action="store_true", default=False,
                        help="Optional. Apply changes to gencfg")

    return parser


def main(options):
    # get all allocations
    allow_owners = set(get_possible_group_owners())
    allocations = []
    for elem in load_nanny_url("v2/resourcesmanager/clusters/")["clusters"]:
        cluster_id = elem["id"]
        if cluster_id == "TEST":
            continue
        for subelem in elem["allocations"]:
            try:
                allocations.append(TNannyAllocation(subelem, cluster_id, allow_owners, options))
            except CanNotBeImportedException, e:
                print red_text(str(e))

    # Fix nanny allocateion names: due to fact that allocation names are not unique we have to add allocation id to group name
    allocations_count = defaultdict(int)
    for allocation in allocations:
        allocations_count[allocation.name] += 1
    for group in CURDB.groups.get_groups(): # check for intersection with already created groups
        if (group.card.master is None) or (group.card.master.card.name != options.group.card.name):
            allocations_count[group.card.name] += 1

    multiple_found_names = filter(lambda x: allocations_count[x] > 1, allocations_count)
    for allocation in allocations:
        if allocation.name in multiple_found_names:
            allocation.name = '%s_%s' % (allocation.name, allocation.allocation_id)


    # some allocations still do not have assigned port
    used_ports = set(filter(lambda x: x is not None, map(lambda x: x.port, allocations)))
    free_ports = filter(lambda x: x not in used_ports, range(1025, 4096))
    if options.verbose >= 1:
        print "Found %d nanny groups with instances on different ports" % (
        len(filter(lambda x: x.port is None, allocations)))
    for allocation in allocations:
        if allocation.port is None:
            allocation.port = free_ports.pop(0)
            if options.verbose >= 2:
                print "   Group %s: set port %s" % (allocation.name, allocation.port)

    # caclulate list of allocateion to add/modify/delete
    nanny_names = set(map(lambda x: x.name, allocations))
    gencfg_names = set(map(lambda x: x.card.name, options.group.slaves))

    to_add = list(nanny_names - gencfg_names)
    to_remove = list(gencfg_names - nanny_names)
    to_modify = map(lambda x: x.name,
                    filter(lambda x: x.name in gencfg_names and (not x.is_equal_to_gencfg()), allocations))

    if options.verbose >= 1:
        print "Total %d nanny groups, to add %s, to remove %s, to modify %s" % (
        len(allocations), len(to_add), len(to_remove), len(to_modify))
        if options.verbose >= 2:
            print "    Added to gencfg: %s" % " ".join(sorted(to_add))
            print "    Removed from gencfg: %s" % " ".join(sorted(to_remove))
            print "    Modified in gencfg: %s" % " ".join(sorted(to_modify))

    # manipulate with groups
    # groups for modification better remove and add than just modify
    for groupname in to_remove + to_modify:
        CURDB.groups.remove_group(groupname)
    for allocation in filter(lambda x: x.name in to_add + to_modify, allocations):
        allocation.import_to_gencfg()

    if options.apply:
        CURDB.groups.update(smart=True)
    else:
        print red_text("Not updated!!! Add option -y to update.")


if __name__ == '__main__':
    options = get_parser().parse_cmd()

    main(options)
