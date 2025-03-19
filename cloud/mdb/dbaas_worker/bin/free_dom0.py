"""
Script for porto dom0 free with move_container
"""

import logging
from concurrent.futures import ThreadPoolExecutor, as_completed
from queue import Queue

from dbaas_common.worker import get_expected_resize_hours

from cloud.mdb.dbaas_worker.internal.config import get_config, worker_args_parser
from cloud.mdb.dbaas_worker.internal.query import execute
from cloud.mdb.dbaas_worker.internal.tools.move_container import MoveContainerExecutor
from cloud.mdb.dbaas_worker.internal.tools.resetup import HostResetuper


def resolve_hosts(base_resetuper, dom0):
    """
    Classify dom0 containers on found/not found in metadb
    """
    matched = []
    unmatched = []

    hosts = base_resetuper.get_porto_containers(dom0)
    with base_resetuper.metadb.get_master_conn() as conn:
        with conn:
            cur = conn.cursor()
            for host in hosts:
                res = execute(cur, 'get_host_info', fqdn=host)
                if res:
                    matched.append(host)
                else:
                    unmatched.append(host)

    return matched, unmatched


def move_host(config, dom0, fqdn):
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
    try:
        executor.mlock.lock_cluster(sorted(args['hosts']))
        transfer = executor.dbm_api.get_container_transfer(fqdn)
        if not transfer:
            executor.run_operation_sync(fqdn, 'run-pre-restart-script', args['hosts'][fqdn]['environment'], timeout=600)
        with executor.ssh.get_conn(dom0) as conn:
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


def free_dom0():
    """
    Console entry-point
    """
    parser = worker_args_parser()
    parser.add_argument('-t', '--threads', type=int, default=16, help='Number of worker threads')
    parser.add_argument('-s', '--skip', action='store_true', help='Skip hosts with unknown type (e.g. CP hosts)')
    parser.add_argument('dom0', type=str, help='target dom0 FQDN')
    args = parser.parse_args()

    logging.basicConfig(level=logging.INFO, format='%(asctime)s %(levelname)s:\t%(message)s')
    config = get_config(args.config)
    base_resetuper = HostResetuper(
        config,
        {
            'task_id': 'dom0-resetup',
            'timeout': 24 * 3600,
            'changes': [],
            'context': {},
            'feature_flags': [],
            'folder_id': 'test',
        },
        Queue(maxsize=10**6),
    )

    matched, unmatched = resolve_hosts(base_resetuper, args.dom0)

    if unmatched and not args.skip:
        raise RuntimeError(f'Not dataplane hosts found: {", ".join(unmatched)}')

    base_resetuper.ssh.setup_agent(config.ssh.private_key)

    with ThreadPoolExecutor(max_workers=args.threads, thread_name_prefix='move-host-') as executor:
        future_to_fqdn = {}
        for host in matched:
            future_to_fqdn[executor.submit(move_host, config, args.dom0, host)] = host
        for future in as_completed(future_to_fqdn):
            fqdn = future_to_fqdn[future]
            try:
                future.result()
            except Exception as exc:
                print(f'{fqdn} failed with {exc!r}')
            else:
                print(f'{fqdn} done')
