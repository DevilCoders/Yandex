"""
Monitoring for orphan resources
"""

import datetime
import logging
import re
from queue import Queue

from cloud.mdb.dbaas_worker.internal.config import get_config, worker_args_parser
from cloud.mdb.dbaas_worker.internal.providers.base_metadb import BaseMetaDBProvider
from cloud.mdb.dbaas_worker.internal.providers.compute import ComputeApi
from cloud.mdb.dbaas_worker.internal.providers.dbm import DBMApi

COMPUTE_CREATE_THRESH = 24 * 3600


class OrphanResourcesMonitor:
    """
    Check extrenal systems for orphan resources
    """

    def __init__(self, config, task, queue):
        self.metadb = BaseMetaDBProvider(config, task, queue)
        self.compute = ComputeApi(config, task, queue)
        self.dbm = DBMApi(config, task, queue)

    def get_metadb_resources(self):
        """
        Get resources existing in metadb (only hosts for now)
        """
        ret = {}
        with self.metadb.get_master_conn() as conn:
            with conn:
                cur = conn.cursor()
                cur.execute(
                    """
                    SELECT h.fqdn
                    FROM dbaas.hosts h
                    JOIN dbaas.subclusters sc ON (h.subcid = sc.subcid)
                    JOIN dbaas.clusters c ON (sc.cid = c.cid)
                    WHERE code.managed(c) AND (code.visible(c) OR c.status = 'DELETE-ERROR'::dbaas.cluster_status)
                    """
                )
                ret['hosts'] = {row['fqdn'] for row in cur.fetchall()}
        return ret

    def get_metadb_host_status(self, fqdn):
        """
        Check if host should exist
        """
        with self.metadb.get_master_conn() as conn:
            with conn:
                cur = conn.cursor()
                cur.execute(
                    """
                    SELECT h.fqdn
                    FROM dbaas.hosts h
                    JOIN dbaas.subclusters sc ON (h.subcid = sc.subcid)
                    JOIN dbaas.clusters c ON (sc.cid = c.cid)
                    WHERE code.managed(c) AND code.visible(c) AND h.fqdn = %(fqdn)s
                    """,
                    {'fqdn': fqdn},
                )
                return bool(cur.fetchall())

    def check_porto(self, exclude):
        """
        Check for orphan resources specific to porto vtype
        """
        exclude = re.compile(exclude) if exclude else None
        ret = {}
        metadb_resources = self.get_metadb_resources()
        containers = {}
        for volume in self.dbm.list_volumes():
            if volume['container'] not in containers:
                containers[volume['container']] = 0
            containers[volume['container']] += 1
        dataplane_containers = [container for container, volumes_count in containers.items() if volumes_count > 1]
        for container in dataplane_containers:
            if 'yandex.net' not in container:
                # Skip transfer placeholders
                continue
            if exclude and exclude.match(container):
                continue
            if container not in metadb_resources['hosts'] and not self.get_metadb_host_status(container):
                if 'container' not in ret:
                    ret['container'] = []
                ret['container'].append(container)
        return ret

    def check_compute(self, exclude, exclude_labels):
        """
        Check for orphan resources specific to compute vtype
        """
        exclude = re.compile(exclude) if exclude else None
        exclude_labels = set(tuple(kv.split(':')) for kv in exclude_labels or [])

        ret = {}
        metadb_resources = self.get_metadb_resources()
        now = datetime.datetime.now()
        for disk in self.compute.list_disks():
            if not disk.instance_ids and (now - disk.created_at).total_seconds() > COMPUTE_CREATE_THRESH:
                if 'disk' not in ret:
                    ret['disk'] = []
                ret['disk'].append(disk.id)
        for instance in self.compute.list_instances(folder_id=None):
            fqdn = instance.fqdn
            if exclude and exclude.match(fqdn):
                continue
            if instance.labels and exclude_labels.intersection(set(instance.labels.items())):
                continue
            if (
                fqdn not in metadb_resources['hosts']
                and (now - instance.created_at).total_seconds() > COMPUTE_CREATE_THRESH
            ):
                if self.get_metadb_host_status(fqdn):
                    continue
                if 'instance' not in ret:
                    ret['instance'] = []
                ret['instance'].append(f'{instance.id} {instance.fqdn}')
        return ret


def orphan_resources():
    """
    Console entry-point
    """
    parser = worker_args_parser()
    parser.add_argument(
        '-v',
        '--vtype',
        default='compute',
        const='compute',
        nargs='?',
        choices=['compute', 'porto'],
        help='Target vtype (compute or porto, default: %(default)s)',
    )
    parser.add_argument('-m', '--monrun', action='store_true', help='Monrun-friendly output')
    parser.add_argument('-d', '--debug', action='store_true', help='Enable logging for debug')
    parser.add_argument('-e', '--exclude', type=str)
    parser.add_argument(
        '-l',
        '--exclude-label',
        nargs='*',
        help='Exclude resources if it has any of given labels (in key:value format)',
    )
    args = parser.parse_args()

    logging.basicConfig(
        level=(logging.DEBUG if args.debug else logging.CRITICAL),
        format='%(asctime)s %(levelname)s:\t%(message)s',
    )
    config = get_config(args.config)
    monitor = OrphanResourcesMonitor(
        config,
        {
            'task_id': 'orphan-resources-monitor',
            'timeout': 24 * 3600,
            'changes': [],
            'context': {},
            'feature_flags': [],
            'folder_id': 'test',
        },
        Queue(maxsize=10**6),
    )

    try:
        if args.vtype == 'compute':
            orphans = monitor.check_compute(args.exclude, args.exclude_label)
        else:
            orphans = monitor.check_porto(args.exclude)
    except Exception as exc:
        if args.monrun:
            print(f'1;Unable to check: {exc!r}')
            return
        raise

    if args.monrun:
        if not orphans:
            print('0;OK')
            return
        msg = ', '.join(sorted([f'{key}: {len(value)}' for key, value in orphans.items()]))
        print(f'1;{msg}')
        return

    for resource_type, resources in orphans.items():
        for resource in resources:
            print(f'{resource_type} {resource}')
