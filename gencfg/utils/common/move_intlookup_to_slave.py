#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import string
from argparse import ArgumentParser

import gencfg
import core.argparse.types as argparse_types
from core.db import CURDB
from core.igroups import CIPEntry


def parse_cmd():
    parser = ArgumentParser(
        description="Move intlookup to slave group (without changing ports). If needed, create this group. Replacement for PostActionReplaceAutoTags from searcherlookup_postactions.py")
    parser.add_argument("-i", "--intlookup", type=argparse_types.intlookup, required=True,
                        help="Obligatory. Source intlookup")
    parser.add_argument("-g", "--slave-group", type=str, required=True,
                        help="Obligatory. New slave group for intlookup")
    parser.add_argument("--int-slave-group", type=str, required=False,
                        help="Optional. Int slave group")
    parser.add_argument("-c", "--create-slave-group", action="store_true", default=False,
                        help="Optional. Create slave group")
    parser.add_argument("-p", "--port", type=str, required=False,
                        help="Optional (obligatory when creating slave group). Port function")
    parser.add_argument("--itype", type=str, required=False,
                        help="Optinal. Itype for new group")
    parser.add_argument("--ctype", type=str, required=False,
                        help="Optional. Ctype for new group")
    parser.add_argument("--prj", type=str, required=False,
                        help="Optional. Prj for new group")

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    options = parser.parse_args()

    if not options.create_slave_group:
        slave_group = CURDB.groups.get_group(options.slave_group)
        # if slave_group.card.master.card.name != options.intlookup.base_type:
        #     raise Exception("Slave group %s is not slave of intlookup group %s" % (options.slave_group, options.intlookup.base_type))
        if len(slave_group.card.intlookups) or len(slave_group.getHosts()):
            raise Exception("Trying to add intlookup to non-empty group %s" % slave_group.card.name)

    return options


def _create_slave_group(groupname, funcs, tags, options):
    master_group = CURDB.groups.get_group(options.intlookup.base_type)
    if master_group.card.master is not None:
        master_group = master_group.card.master

    CURDB.groups.add_group(groupname,
                           description="Slave group, created for intlookup %s" % options.intlookup.file_name,
                           master=master_group.card.name,
                           owners=master_group.card.owners,
                           funcs=funcs,
                           tags=tags,
                           donor=None
                           )
    if not options.port:  # share ports with master group
        CURDB.groups.get_group(groupname).properties.share_master_ports = True


def main(options):
    src_group = CURDB.groups.get_group(options.intlookup.base_type)
    src_group.mark_as_modified()
    tags = {
        'itype': options.itype if options.itype else src_group.card.tags.itype,
        'ctype': options.ctype if options.ctype else src_group.card.tags.ctype,
        'prj': [options.prj] if options.prj else src_group.card.tags.prj,
    }

    # create slave group if needed
    if options.create_slave_group is True:
        funcs = {
            'instanceCount': src_group.card.legacy.funcs.instanceCount,
            'instancePower': src_group.card.legacy.funcs.instancePower,
            'instancePort': options.port if options.port else src_group.card.legacy.funcs.instancePort,
        }

        _create_slave_group(options.slave_group, funcs, tags, options)
        if options.int_slave_group:
            src_int_group = string.replace(options.intlookup.get_int_instances()[0].type, '_BASE', '_INT')
            src_int_group = CURDB.groups.get_group(src_int_group)

            tags['itype'] = src_int_group.card.tags.itype
            funcs = {
                'instanceCount': src_int_group.card.legacy.funcs.instanceCount,
                'instancePower': src_int_group.card.legacy.funcs.instancePower,
                'instancePort': src_int_group.card.legacy.funcs.instancePort,
            }
            _create_slave_group(options.int_slave_group, funcs, tags, options)

    slave_group = CURDB.groups.get_group(options.slave_group)
    if src_group.custom_instance_power_used:
        slave_group.custom_instance_power_used = True
    slave_group.mark_as_modified()

    hosts = list(set(map(lambda x: x.host, options.intlookup.get_base_instances())))
    for host in hosts:
        slave_group.addHost(host)
    if options.int_slave_group:
        int_slave_group = CURDB.groups.get_group(options.int_slave_group)
        int_slave_group.mark_as_modified()
        hosts = list(set(map(lambda x: x.host, options.intlookup.get_int_instances())))
        for host in hosts:
            int_slave_group.addHost(host)

    CURDB.intlookups.disconnect(options.intlookup.file_name)

    def fbase(instance):
        newinstance = CURDB.groups.get_instance_by_N(instance.host.name, options.slave_group, instance.N)
        newinstance.power = instance.power
        return newinstance

    options.intlookup.replace_instances(fbase, run_base=True, run_int=False, run_intl2=False)
    options.intlookup.base_type = options.slave_group
    options.intlookup.mark_as_modified()

    if options.int_slave_group:
        def fint(instance):
            newinstance = CURDB.groups.get_instance_by_N(instance.host.name, options.int_slave_group, instance.N)
            newinstance.power = instance.power
            return newinstance

        options.intlookup.replace_instances(fint, run_base=False, run_int=True, run_intl2=False)

    CURDB.intlookups.connect(options.intlookup.file_name)

    CURDB.update(smart=True)


if __name__ == '__main__':
    options = parse_cmd()
    main(options)
