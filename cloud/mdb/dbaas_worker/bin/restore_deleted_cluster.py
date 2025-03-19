"""
Script to restore deleted porto cluster
"""

import logging
from queue import Queue

from dateutil import parser as dt_parser

from cloud.mdb.dbaas_worker.internal.config import get_config, worker_args_parser
from cloud.mdb.dbaas_worker.internal.providers.solomon_service_alerts import SolomonServiceAlerts
from cloud.mdb.dbaas_worker.internal.providers.tls_cert import TLSCert
from cloud.mdb.dbaas_worker.internal.tasks.utils import build_host_group, issue_tls_wrapper
from cloud.mdb.dbaas_worker.internal.tools.resetup import HostResetuper
from cloud.mdb.dbaas_worker.internal.utils import get_conductor_root_group, get_first_value, get_image_by_major_version

from .conductor_migrate import GROUP_RESOLVE_MAP, ConductorMigrator


def initzk_postgresql(resetuper, hosts):
    """
    Run initzk on hosts (data from zookeeper is deleted in cluster_delete)
    """
    resetuper.deploy_api.wait(
        [
            resetuper.deploy_api.run(
                name=host,
                method={
                    'commands': [
                        {'type': 'cmd.run', 'arguments': [f'timeout 5 pgsync-util initzk {host}'], 'timeout': 600},
                    ],
                    'fqdns': [host],
                    'parallel': 1,
                    'stopOnErrorCount': 1,
                    'timeout': 600,
                },
                deploy_title=f'initzk-{host}',
            )
            for host in hosts
        ],
    )


PRE_HS_HOOKS = {
    'postgresql_cluster': initzk_postgresql,
}


def discover_backups(resetuper, hosts):
    """
    Find volume backups for hosts
    """
    res = {}
    for host, opts in hosts.items():
        if opts['vtype'] != 'porto':
            raise RuntimeError(f'{host} is not a porto host')
        backups = list(resetuper.dbm.paginate('api/v2/volume_backups/', {'container': host, 'path': '/'}))
        if not backups:
            raise RuntimeError(f'No volume backups for {host}')
        latest = {'dom0': None, 'ts': 0}
        for backup in backups:
            parsed = dt_parser.parse(backup['create_ts']).timestamp()
            if parsed > latest['ts']:
                latest = {'dom0': backup['dom0'], 'ts': parsed}
        res[host] = latest['dom0']
    return res


def revert_metadb_delete(resetuper, cid):
    """
    Drop cluster delete tasks and revert revisions to latest successful task
    """
    with resetuper.metadb.get_master_conn() as conn:
        with conn:
            cur = conn.cursor()
            cur.execute(
                """
                SELECT c.*
                FROM dbaas.clusters cl
                JOIN dbaas.folders f ON (f.folder_id = cl.folder_id)
                JOIN dbaas.clouds c ON (c.cloud_id = f.cloud_id)
                WHERE cl.cid = %(cid)s
                FOR NO KEY UPDATE
                """,
                {'cid': cid},
            )
            res = cur.fetchall()
            if not res:
                raise RuntimeError('Unable to lock cloud')
            cloud = res[0]
            cur.execute(
                """
                DELETE FROM dbaas.worker_queue_restart_history
                WHERE cid = %(cid)s
                AND (task_type LIKE '%%_cluster_delete'
                     OR task_type LIKE '%%_cluster_delete_metadata'
                     OR task_type LIKE '%%_cluster_purge')
                """,
                {'cid': cid},
            )
            cur.execute(
                """
                DELETE FROM dbaas.worker_queue
                WHERE cid = %(cid)s
                AND (task_type LIKE '%%_cluster_delete'
                     OR task_type LIKE '%%_cluster_delete_metadata'
                     OR task_type LIKE '%%_cluster_purge')
                """,
                {'cid': cid},
            )
            cur.execute(
                """
                SELECT finish_rev
                FROM dbaas.worker_queue
                WHERE cid = %(cid)s
                AND result = true
                AND unmanaged = false
                AND finish_rev IS NOT NULL
                ORDER BY finish_rev DESC
                LIMIT 1
                """,
                {'cid': cid},
            )
            res = cur.fetchall()
            if not res:
                raise RuntimeError('Unable to find a safe revision to revert cluster')
            cur.execute(
                """
                SELECT code.revert_cluster_to_rev(%(cid)s, %(rev)s, %(cid)s || ' restore')
                """,
                {'cid': cid, 'rev': res[0]['finish_rev']},
            )
            cur.execute(
                """
                SELECT code.fix_cloud_usage(%(cloud_ext_id)s)
                """,
                {'cloud_ext_id': cloud['cloud_ext_id']},
            )


def restore_container(resetuper, fqdn, opts, dom0):
    """
    Create porto container from volume backups
    """
    data_dom0_path = f'/data/{fqdn}/data' if opts['disk_type_id'] == 'local-ssd' else '/disks'
    change = resetuper.dbm.container_exists(
        opts['cid'],
        fqdn,
        opts['geo'],
        options={
            'cpu_limit': opts['cpu_limit'],
            'cpu_guarantee': opts['cpu_guarantee'],
            'memory_limit': opts['memory_limit'],
            'memory_guarantee': opts['memory_guarantee'],
            'net_limit': opts['network_limit'],
            'net_guarantee': opts['network_guarantee'],
            'io_limit': opts['io_limit'],
            'project_id': opts['subnet_id'] or resetuper.config.dbm.project_id,
            'managing_project_id': resetuper.config.dbm.managing_project_id,
            'restore': True,
            'dom0': dom0,
        },
        volumes={
            '/': {
                'space_limit': opts['props'].rootfs_space_limit,
                'dom0_path': f'/data/{fqdn}/rootfs',
            },
            opts['props'].dbm_data_path: {
                'space_limit': opts['space_limit'],
                'dom0_path': data_dom0_path,
            },
        },
        bootstrap_cmd=get_image_by_major_version(
            image_template=getattr(opts['props'], 'dbm_bootstrap_cmd_template', None),
            image_fallback=opts['props'].dbm_bootstrap_cmd,
            task_args={},
        ),
        platform_id=opts['platform_id'],
        secrets={},
        revertable=False,
    )
    return change.jid if change else None


def restore_tls(resetuper, cid, hosts):
    """
    Reissue certs
    """
    tls_cert = TLSCert(resetuper.config, resetuper.task, resetuper.queue)
    for host, opts in hosts.items():
        group = build_host_group(opts['props'], {host: opts})
        issue_tls_wrapper(cid, group, tls_cert)


def restore(resetuper, cid):
    """
    Restore porto cluster with host resetuper
    """
    args = resetuper.get_base_task_args(cid, None)
    for opts in args['hosts'].values():
        opts['cid'] = cid

    host_dom0_map = discover_backups(resetuper, args['hosts'])

    revert_metadb_delete(resetuper, cid)

    for host in args['hosts']:
        resetuper.drop_deploy_data(host)
        resetuper.juggler_api.downtime_exists(host)

    cluster_type = get_first_value(args['hosts'])['cluster_type']

    migrator = ConductorMigrator(resetuper.config, cluster_type)

    resolver = GROUP_RESOLVE_MAP.get(cluster_type, migrator.default_resolve)

    for group, data in resolver(migrator.config, args['hosts']).items():
        migrator.conductor.group_exists(group, get_conductor_root_group(data['props'], data['opts']))
        for host, opts in args['hosts'].items():
            if opts['subcid'] == data['opts']['subcid']:
                migrator.conductor.host_exists(host, opts['geo'], group)
                opts['props'] = data['props']

    jids = []

    for host, opts in args['hosts'].items():
        jid = restore_container(resetuper, host, opts, host_dom0_map[host])
        if jid:
            jids.append(jid)

    resetuper.deploy_api.wait(jids)

    restore_tls(resetuper, cid, args['hosts'])

    pre_hs_hook = PRE_HS_HOOKS.get(cluster_type)

    if pre_hs_hook:
        pre_hs_hook(resetuper, args['hosts'])

    resetuper.deploy_api.wait(
        [resetuper.deploy_api.run(host, pillar={'service-restart': True}) for host in args['hosts']],
    )

    solomon = SolomonServiceAlerts(resetuper.config, resetuper.task, resetuper.queue)
    solomon.state_to_desired(cid)


def restore_deleted_cluster():
    """
    Console entry-point
    """
    parser = worker_args_parser()
    parser.add_argument('cid', type=str, help='target cluster id')
    args = parser.parse_args()

    logging.basicConfig(level=logging.INFO, format='%(asctime)s %(levelname)s:\t%(message)s')
    config = get_config(args.config)
    resetuper = HostResetuper(
        config,
        {
            'task_id': f'{args.cid}-restore',
            'timeout': 24 * 3600,
            'changes': [],
            'context': {},
            'feature_flags': [],
            'folder_id': 'test',
        },
        Queue(maxsize=10**6),
    )
    restore(resetuper, args.cid)
