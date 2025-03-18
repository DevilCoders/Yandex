#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

from argparse import ArgumentParser
import re
import copy

import gencfg
from core.db import CURDB
import core.argparse.types as argparse_types


def create_slave_group(parent_group, slave_group_name, description, group_funcs=None, group_tags=None):
    if parent_group.master is None:
        master_group = parent_group
    else:
        master_group = parent_group.master

    my_funcs = {
        'instanceCount': 'exactly1',
        'instancePower': 'zero',
        'instancePort': 'old18080',
    }
    if group_funcs is not None:
        my_funcs.update(group_funcs)

    my_tags = {
        'ctype': parent_group.tags.ctype,
        'itype': parent_group.tags.itype,
        'prj': copy.copy(parent_group.tags.prj),
        'metaprj': parent_group.tags.metaprj,
    }
    if group_tags is not None:
        my_tags.update(group_tags)

    my_group = CURDB.groups.add_group(slave_group_name, description="%s (automatically created for %s)" % (
    description, parent_group.name),
                                      owners=copy.copy(parent_group.owners), watchers=copy.copy(parent_group.watchers),
                                      funcs=my_funcs,
                                      master=master_group.name, donor=parent_group.name, tags=my_tags)

    return my_group


class AddDataupdaterGroup(object):
    @staticmethod
    def add_subparser(subparsers):
        myparser = subparsers.add_parser('add_dataupdater_group',
                                         help='Added dataupater group to specified nmeta group')
        myparser.add_argument('-g', '--nmeta-group', type=argparse_types.group,
                              help='Obligatory. Nmeta group')

        myparser.set_defaults(normalize=AddDataupdaterGroup.normalize, main=AddDataupdaterGroup.main)

    @staticmethod
    def normalize(options):
        if options.nmeta_group.tags.itype != 'upper':
            raise Exception("Group %s is not upper" % options.nmeta_group.name)

        if len(re.findall('NMETA', options.nmeta_group.name)) != 1:
            raise Exception("Group %s must contain exactly one <NMETA>" % options.nmeta_group.name)

    @staticmethod
    def main(options):
        nmeta_group = options.nmeta_group

        if nmeta_group.master is None:
            master_group = nmeta_group
        else:
            master_group = nmeta_group.master

        my_group_name = re.sub('NMETA', 'DATAUPDATER', nmeta_group.name)
        my_funcs = {
            'instanceCount': 'exactly1',
            'instancePower': 'zero',
            'instancePort': 'old18080',
        }
        my_tags = {
            'ctype': nmeta_group.tags.ctype,
            'itype': 'dataupdaterd',
            'prj': copy.copy(nmeta_group.tags.prj),
            'metaprj': nmeta_group.tags.metaprj,
        }

        my_group = CURDB.groups.add_group(my_group_name,
                                          description="Dataupdater, automatically created for %s" % nmeta_group.name,
                                          owners=copy.copy(nmeta_group.owners),
                                          watchers=copy.copy(nmeta_group.watchers), funcs=my_funcs,
                                          master=master_group.name, donor=nmeta_group.name, tags=my_tags)
        my_group.reqs.instances.memory_guarantee = '600 Mb'

        from utils.pregen import generate_trivial_intlookup
        sub_options = {
            'groups': [my_group],
            'bases_per_group': 1,
            'shard_count': 'DataupdaterdTier0',
        }
        sub_options = generate_trivial_intlookup.get_parser().parse_json(sub_options)
        generate_trivial_intlookup.normalize(sub_options)
        generate_trivial_intlookup.main(sub_options)


class AddIntGroup(object):
    @staticmethod
    def add_subparser(subparsers):
        myparser = subparsers.add_parser('add_int_group', help='Added int group to specified base group')
        myparser.add_argument('-g', '--base-group', type=argparse_types.group,
                              help='Obligatory. Nmeta group')

        myparser.set_defaults(normalize=AddIntGroup.normalize, main=AddIntGroup.main)

    @staticmethod
    def normalize(options):
        base_types = ['fusion', 'base']
        if options.base_group.tags.itype not in base_types:
            raise Exception("Group %s type is not in %s" % (options.base_group.tags.itype, ",".join(base_types)))

        if len(re.findall('BASE', options.base_group.name)) != 1:
            raise Exception("Group %s must contain exactly one <BASE>" % options.base_group.name)

        if options.base_group.card.legacy.funcs.instancePort == 'default':
            raise Exception("Can not create int group for base group with instancePort func <default>")

    @staticmethod
    def main(options):
        int_group_name = re.sub('BASE', 'INT', options.base_group.name)

        m = re.match('(.*?)([0-9]+)', options.base_group.card.legacy.funcs.instancePort)
        if m is None:
            raise Exception("Instance func <%s> in group %s can not be parsed" % (options.base_group.card.legacy.funcs.instancPort, options.base_group.name))
        int_group_instance_port = "%s%s" % (m.group(1), int(m.group(2)) - 100)

        int_group_funcs = {
            'instancePort': int_group_instance_port,
        }

        int_group_tags = {
            'itype': 'int',
        }

        create_slave_group(options.base_group, int_group_name, "Int group", group_funcs=int_group_funcs,
                           group_tags=int_group_tags)


def parse_cmd():
    parser = ArgumentParser(description="Execute some special action")
    subparsers = parser.add_subparsers(help='Special actions help')

    AddDataupdaterGroup.add_subparser(subparsers)
    AddIntGroup.add_subparser(subparsers)

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    options = parser.parse_args()

    return options


def normalize(options):
    options.normalize(options)


def main(options):
    options.main(options)

    CURDB.update()


if __name__ == '__main__':
    options = parse_cmd()
    normalize(options)
    main(options)
