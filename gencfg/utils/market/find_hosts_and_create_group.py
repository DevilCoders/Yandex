#!/skynet/python/bin/python
# -*- coding: utf-8 -*-
"""This script finds hosts satisfied requirements and creates a new group with these hosts."""

import json
import logging
import os
import sys
from urllib import urlencode

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import gencfg  # noqa

from core.db import CURDB
from core.argparse.parser import ArgumentParserExt
import core.argparse.types as argparse_types
import utils.common.manipulate_itypes as manipulate_itypes
import utils.common.manipulate_volumes as manipulate_volumes
import utils.common.update_igroups as update_igroups
import utils.pregen.find_most_memory_free_machines as find_most_memory_free_machines

LOCATIONS = ['SAS', 'VLA', 'IVA', 'MAN']
CTYPES = ['testing', 'prestable', 'production']
MASTER_GROUP_ENVS = {'testing': 'TEST', 'prestable': 'PROD', 'production': 'PROD'}

logger = logging.getLogger(__name__)
logger.addHandler(logging.StreamHandler())
logger.setLevel(logging.INFO)


def detect_master_groups(group_name, ctype):
    """Detects a master group for the *group* and the *ctype*.

    :type group_name: str
    :rtype: core.igroups.IGroup
    """
    location = group_name.split('_')[0]
    if location not in LOCATIONS:
        raise Exception('Unknown location {}, the list of known locations {}'.format(location, ', '.join(LOCATIONS)))
    return CURDB.groups.get_group('{}_MARKET_{}_GENERAL'.format(location, MASTER_GROUP_ENVS[ctype]))


def prepare_properties(options):
    """Prepares properties for the group created.

    :rtype: str
    """
    return ','.join([
        'dispenser.project_key=marketito',
        'reqs.instances.memory_guarantee={}Gb'.format(options.memory),
        'properties.hbf_parent_macros=_GENCFG_MARKET_{}_'.format(MASTER_GROUP_ENVS[options.ctype]),
        'properties.hbf_resolv_conf=',
    ])


def prepare_tags(options):
    """Prepare tags for the group created.

    :rtype: dict
    """
    return dict(metaprj='market', itype=options.itype, ctype=options.ctype, prj=[options.prj])


def get_parser():
    parser = ArgumentParserExt(
        description=__doc__)
    parser.add_argument('--master-group', type=argparse_types.group, default=None,
                        help='Optional. Master group name')
    parser.add_argument('--group', type=str, required=True,
                        help='Obligatory. Group name to create')
    parser.add_argument('--description', type=str, required=True, help='Obligatory. Description')
    parser.add_argument('--memory', type=float, required=True, help='Obligatory. Memory in GB')
    parser.add_argument('--power', type=int, required=True, help='Obligatory. CPU power')
    parser.add_argument('--instances', type=int, required=True, help='Obligatory. Number of instances')
    parser.add_argument('--ctype', type=str, required=True, choices=CTYPES, help='Obligatory. ctype')
    parser.add_argument('--itype', type=str, required=True, help='Obligatory. itype')
    parser.add_argument('--prj', type=str, required=True, help='Obligatory. prj')
    # TODO: make obligatory
    parser.add_argument('--volume-workdir', type=int, default=3, required=False,
                        help='Size of volume $BSCONFIG_IDIR in GB')
    parser.add_argument('--volume-root', type=int, default=3, required=False, help='Size of volume / in GB')
    parser.add_argument('--volume-logs', type=int, default=30, required=False,
                        help='Size of volume /logs in GB')
    parser.add_argument('--volume-persistent', type=int, default=2, required=False,
                        help='Size of volume /persistent-data in GB')
    parser.add_argument('--volume-cores', type=int, default=0, required=False, help='Size of volume /cores in GB')

    return parser


def normalize(options):
    if options.master_group is None:
        options.master_group = detect_master_groups(group_name=options.group, ctype=options.ctype)


def main(options):
    if CURDB.groups.has_group(options.group):
        logger.info('Group %s already exists. Do nothing', options.group)
        return

    # stage 0: create itype
    logger.info('Creating an itype "%s"', options.itype)
    itype_options = manipulate_itypes.get_parser().parse_json(
        dict(action='add', itype=options.itype, descr=options.description)
    )
    manipulate_itypes.normalize(itype_options)
    manipulate_itypes.main(itype_options)

    # stage 1: find hosts
    logger.info('Looking for suitable hosts')
    master_group_hosts = find_most_memory_free_machines.jsmain(
        dict(action='show', hosts=options.master_group.getHosts())
    )
    disk_space = options.volume_logs + options.volume_cores + options.volume_persistent + \
                 options.volume_workdir + options.volume_root

    satisfied_hosts = [
        h.host for h in master_group_hosts if
        h.resources.power >= options.power and
        h.resources.mem >= options.memory and
        h.resources.hdd >= disk_space
    ]

    if len(satisfied_hosts) < options.instances:
        number_of_host_to_add = options.instances - len(satisfied_hosts)
        params = {
            'summary': 'Добавить ещё как минимум {} машин в группу {}'.format(
                number_of_host_to_add,
                options.master_group.card.name
            ),
            'description': (
                    'Не нашлось машин для размещение сервиса со следующими запросами:\n' +
                    '- CPU {} cores\n'.format(options.power / 40) +
                    '- Memory {} GB\n'.format(options.memory) +
                    '- Disk {} GB\n\n'.format(disk_space) +
                    'https://wiki.yandex-team.ru/market/sre/checklist/dobavit-vneplanovo-zheleznyx-mashin-v-rtc/'
            ),
            'type': 2,
            'priority': 2,
            'tags': 'cs_duty',
            'queue': 'CSADMIN',
        }

        create_ticket_link = 'https://st.yandex-team.ru/createTicket?' + urlencode(params)

        msg = ('Found only {satisfied_hosts_number} suitable hosts in {master_group_name}, '
               'requested {instances_number}. '
               'Please create a ticket using the following link {create_ticket_link}.').format(
            satisfied_hosts_number=len(satisfied_hosts),
            master_group_name=options.master_group.card.name,
            instances_number=options.instances,
            create_ticket_link=create_ticket_link,
        )
        raise Exception(msg)

    satisfied_hosts = satisfied_hosts[:options.instances]

    # stage 2: create group
    logger.info('Creating a group "%s"', options.group)
    update_igroups.jsmain(dict(
        action='addgroup',
        db=CURDB,
        description=options.description,
        group=options.group,
        hosts=satisfied_hosts,
        instance_count_func='exactly1',
        instance_port_func='auto',
        instance_power_func='exactly{}'.format(options.power),
        parent_group=options.master_group,
        properties=prepare_properties(options),
        tags=prepare_tags(options),
    ))

    # stage 3: configure volumes
    logger.info(
        'Configuring volumes "" %dGB, "/" %dGB, "/logs" %dGB, "/persistent-data" %dGB, "/cores" %dGB',
        options.volume_workdir,
        options.volume_root,
        options.volume_logs,
        options.volume_persistent,
        options.volume_cores
    )
    volumes_options = manipulate_volumes.get_parser().parse_json(
        dict(
            action='put',
            groups=options.group,
            json_volumes=json.dumps([
                {
                    "guest_mp": "",
                    "host_mp_root": "/place",
                    "mount_point_workdir": True,
                    "quota": "{} Gb".format(options.volume_workdir)
                },
                {
                    "guest_mp": "/",
                    "host_mp_root": "/place",
                    "quota": "{} Gb".format(options.volume_root)
                },
                {
                    "guest_mp": "/logs",
                    "host_mp_root": "/place",
                    "quota": "{} Gb".format(options.volume_logs)
                },
                {
                    "guest_mp": "/persistent-data",
                    "host_mp_root": "/place",
                    "quota": "{} Gb".format(options.volume_persistent)
                },
                {
                    "guest_mp": "/cores",
                    "host_mp_root": "/place",
                    "quota": "{} Gb".format(options.volume_cores)
                },
            ]),
        )
    )
    manipulate_volumes.normalize(volumes_options)
    manipulate_volumes.main(volumes_options)


if __name__ == '__main__':
    options = get_parser().parse_cmd()

    normalize(options)

    main(options)
