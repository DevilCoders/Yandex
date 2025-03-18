#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import copy
from argparse import ArgumentParser

import gencfg
from core.db import CURDB
from gaux.aux_portovm import gen_guest_group_name, gen_guest_group, gen_vm_host_name, gen_vm_host
from core.settings import SETTINGS

import utils.pregen.update_hosts


class EActions(object):
    CHECK = 'check'
    UPDATE = 'update'
    ALL = [CHECK, UPDATE]


def parse_cmd():
    parser = ArgumentParser(description="Check presence of/Add virtual machines corresponding to proton groups")
    parser.add_argument("-a", "--action", type=str, required=True,
                        choices=EActions.ALL,
                        help="Obligatory. Action to execute")
    parser.add_argument("-v", "--verbose", action="count", default=0,
                        help="Optional. Increase output verbosity (maximum 2)")

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    options = parser.parse_args()

    return options


def main(options):
    vm_groups = [group for group in CURDB.groups.get_groups() if
                 group.card.tags.itype in (SETTINGS.constants.portovm.itype, 'psi')]
    vmguest_groups = [group for group in CURDB.groups.get_groups() if
                      group.card.properties.created_from_portovm_group is not None]

    if options.action == EActions.UPDATE:
        # create VMGUEST master groups
        guest_groups = []
        for host_group in vm_groups:
            if options.verbose > 0:
                print 'Found master group %s' % host_group.card.name

            guest_group_name = gen_guest_group_name(host_group.card.name)
            if not CURDB.groups.has_group(guest_group_name):
                guest_group = gen_guest_group(host_group)
                if options.verbose > 0:
                    print "* added group %s" % guest_group.card.name
            else:
                guest_group = CURDB.groups.get_group(guest_group_name)

            new_guest_machines = []
            for instance in host_group.get_instances():
                guest_machine = gen_vm_host(CURDB, instance)
                new_guest_machines.append(guest_machine)

            old_guest_machines = guest_group.getHosts()

            to_add = set(new_guest_machines) - set(old_guest_machines)
            to_remove = set(old_guest_machines) - set(new_guest_machines)

            if len(to_add) > 0 or len(to_remove) > 0:
                guest_group.mark_as_modified()
                CURDB.hosts.mark_as_modified()

            if options.verbose > 1 or (len(to_add) + len(to_remove)) > 0:
                print "Group %s, to_add %d, to_remove %s" % (guest_group_name, len(to_add), len(to_remove))

            CURDB.groups.move_hosts(list(to_add), guest_group)

            for host in to_remove:
                guest_group.removeHost(host)
                CURDB.hosts.remove_host(host)
                if options.verbose > 1:
                    print "* destroyed host %s" % host.name

            guest_groups.append(guest_group)

        if options.verbose > 1:
            for guest_group in guest_groups:
                print 'Group %s, total %s hosts: %s' % (
                    guest_group.card.name,
                    len(guest_group.getHosts()),
                    '...'
                )

        CURDB.update(smart=True)

        return 0
    elif options.action == EActions.CHECK:
        failed = False

        # check if all vmguest machines in vmguest groups
        vmguest_machines_from_groups = set(sum(map(lambda x: x.getHosts(), vmguest_groups), []))
        vmguest_machines_from_gencfg = set(filter(lambda x: x.is_vm_guest(), CURDB.hosts.get_all_hosts()))

        diff = list(filter(lambda x: not x.is_vm_guest(), vmguest_machines_from_groups))
        if len(diff) > 0:
            print "Not virtual machines in vmguest groups: %s" % ",".join(map(lambda x: x.name, diff))
            failed = True

        diff = list(vmguest_machines_from_gencfg - vmguest_machines_from_groups)
        if len(diff) > 0:
            print "Virtual machines outside of vmguest groups (%d total): %s" % (
            len(diff), ",".join(map(lambda x: x.name, diff[:100])))
        #            failed = True

        # check if every vm group has corresponding vmguest group with same hosts
        for group in vm_groups:
            vmguest_group_name = gen_guest_group_name(group.card.name)
            if not CURDB.groups.has_group(vmguest_group_name):
                print "Not found vmguest group %s for group %s" % (vmguest_group_name, group.card.name)
                failed = True
                continue

            vmguest_group = CURDB.groups.get_group(vmguest_group_name)
            vmguest_real_hosts = set(map(lambda x: x.name, vmguest_group.getHosts()))
            vmguest_pot_hosts = set(map(lambda x: gen_vm_host_name(CURDB, x), group.get_instances()))

            diff = list(vmguest_real_hosts - vmguest_pot_hosts)
            if len(diff) > 0:
                print "Vmguest group %s has extra %d hosts: %s" % (
                vmguest_group.card.name, len(diff), ",".join(diff[:10]))
                failed = True
            diff = list(vmguest_pot_hosts - vmguest_real_hosts)
            if len(diff) > 0:
                print "Vmguest group %s missing %d hosts: %s" % (
                vmguest_group.card.name, len(diff), ",".join(diff[:10]))
                failed = True

        return int(failed)

    else:
        raise Exception("Unknown action %s" % options.action)


if __name__ == '__main__':
    options = parse_cmd()

    retcode = main(options)

    sys.exit(retcode)
