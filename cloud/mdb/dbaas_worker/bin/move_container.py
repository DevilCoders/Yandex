"""
Wrapper for move_container
"""

import logging
from queue import Queue

from cloud.mdb.dbaas_worker.internal.config import get_config, worker_args_parser
from cloud.mdb.dbaas_worker.internal.tools.move_container import MoveContainerExecutor
from cloud.mdb.dbaas_worker.internal.tools.resetup import HostResetuper
from dbaas_common.worker import get_expected_resize_hours


def move_host(config, fqdn):
    """
    Move single host
    """
    resetuper = HostResetuper(
        config,
        {
            'task_id': f'{fqdn}-move',
            'timeout': 24 * 3600,
            'changes': [],
            'context': {},
            'feature_flags': [],
            'folder_id': 'test',
        },
        Queue(maxsize=10**6),
    )

    info = resetuper.get_host_info(fqdn)
    args = resetuper.get_base_task_args(info['cid'], None)
    resetuper.task['cid'] = info['cid']
    executor = MoveContainerExecutor(config, resetuper.task, resetuper.queue, args)
    container = executor.dbm_api.get_container(fqdn)
    if not container:
        raise RuntimeError(f'Unable to find {fqdn} in dbm')
    dom0 = container['dom0']
    try:
        executor.mlock.lock_cluster(sorted(args['hosts']))
        transfer = executor.dbm_api.get_container_transfer(fqdn)
        if not transfer:
            executor.run_operation_sync(fqdn, 'run-pre-restart-script', args['hosts'][fqdn]['environment'], timeout=600)
        with executor.ssh.get_conn(dom0, use_agent=True) as conn:
            executor.ssh.exec_command(dom0, conn, f'/usr/sbin/move_container.py {fqdn}')
        executor.update_other_hosts_metadata(fqdn)
        timeout = int(get_expected_resize_hours(args['hosts'][fqdn]['space_limit'], 1, 1200).total_seconds())
        executor.run_operation_sync(
            fqdn,
            'run-post-restart-script',
            args['hosts'][fqdn]['environment'],
            pillar={'post-restart-timeout': timeout},
            timeout=timeout,
        )
    finally:
        executor.mlock.unlock_cluster()


def move_container():
    """
    Console entry-point
    """
    parser = worker_args_parser()
    parser.add_argument('fqdn', type=str, help='target container FQDN')
    args = parser.parse_args()

    logging.basicConfig(level=logging.INFO, format='%(asctime)s %(levelname)s:\t%(message)s')
    config = get_config(args.config)

    move_host(config, args.fqdn)
