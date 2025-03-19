"""
Clickhouse Resetup host executor
"""
from typing import Optional

from cloud.mdb.dbaas_worker.internal.environment import EnvironmentName as EnvName
from cloud.mdb.dbaas_worker.internal.exceptions import ExposedException
from cloud.mdb.dbaas_worker.internal.providers.base_metadb import BaseMetaDBProvider, execute
from cloud.mdb.dbaas_worker.internal.providers.iam_jwt import IamJwt
from cloud.mdb.dbaas_worker.internal.providers.internal_api import InternalApi
from cloud.mdb.dbaas_worker.internal.tasks.common.base import BaseExecutor
from cloud.mdb.dbaas_worker.internal.tasks.common.cluster.start import ClusterStartExecutor
from cloud.mdb.dbaas_worker.internal.tasks.common.cluster.stop import ClusterStopExecutor
from cloud.mdb.dbaas_worker.internal.tasks.common.create import BaseCreateExecutor
from cloud.mdb.dbaas_worker.internal.tasks.common.resetup_common import (
    PortoOnlineResetupHost,
    ResetupActionType,
    ComputeOnlineResetupHost,
    ComputeOfflineResetupHost,
    Resetup,
    init_routine,
    resetup_args_from_task_args,
)
from cloud.mdb.dbaas_worker.internal.tasks.utils import register_executor
from .cluster.restore import ClickHouseClusterRestore as ClusterRestoreTask, RESTORE
from .cluster.start import ClickhouseClusterStart as ClusterStartTask
from .cluster.stop import ClickhouseClusterStop as ClusterStopTask
from .host.create import ClickHouseHostCreate as ClickhouseHostCreateTask
from .host.create import ClickHouseZookeeperHostCreate as ZookeeperHostCreateTask


def args_for_host_create(resetuper: Resetup, cid: str, fqdn: str, metadb: BaseMetaDBProvider) -> dict:
    args = resetuper.get_base_task_args(cid, resetuper.ignore_hosts)

    subcid = args['hosts'][fqdn]['subcid']
    shard_id = args['hosts'][fqdn]['shard_id']

    args['resetup_from_replica'] = True
    if len([x for x in args['hosts'] if args['hosts'][x]['shard_id'] == shard_id]) == 1:
        raise RuntimeError('Adding hosts into shard with one host will fail')
    zk_hosts = [x for x in args['hosts'] if 'zk' in args['hosts'][x]['roles']]
    if zk_hosts:
        args['zk_hosts'] = ','.join([f'{host}:2181' for host in zk_hosts])
        if 'zk' in args['hosts'][fqdn]['roles']:
            args['zid_added'] = get_zookeeper_id(metadb, subcid, fqdn)

    args['host'] = fqdn
    args['subcid'] = subcid
    args['shard_id'] = shard_id
    args['service_account_id'] = get_service_account_id(metadb, subcid)
    args['node_role'] = 'zk' if fqdn in zk_hosts else 'clickhouse'
    return args


def get_zookeeper_id(metadb: BaseMetaDBProvider, subcid: str, fqdn: str) -> str:
    """
    Get zookeeper id for host in cluster
    """
    with metadb.get_master_conn() as conn:
        with conn:
            cur = conn.cursor()
            res = execute(
                cur, 'get_pillar', path=['data', 'zk', 'nodes'], cid=None, subcid=subcid, fqdn=None, shard_id=None
            )
            return res[0]['value'][fqdn]


def get_service_account_id(metadb: BaseMetaDBProvider, subcid: str) -> Optional[str]:
    """
    Get service account id if exists
    """
    with metadb.get_master_conn() as conn:
        with conn:
            cur = conn.cursor()
            res = execute(
                cur, 'get_pillar', path='{data,service_account_id}', cid=None, subcid=subcid, shard_id=None, fqdn=None
            )
            return res[0]['value']


def args_for_cluster_create(resetuper: Resetup, cid: str, fqdn: str, admin_api: InternalApi, iam: IamJwt):
    args = resetuper.get_base_task_args(cid, resetuper.ignore_hosts)
    args['host'] = fqdn
    shard_id = args['hosts'][fqdn]['shard_id']

    if len([hostname for hostname, host_info in args['hosts'].items() if host_info['shard_id'] == shard_id]) != 1:
        raise ExposedException('Restoring shard with more than one host will fail')
    for host, opts in args['hosts'].copy().items():
        if opts.get('shard_id') is not None and opts.get('shard_id') != shard_id:
            del args['hosts'][host]

    args.update(
        {
            's3_bucket': resetuper.get_s3_bucket(cid),
            'target-pillar-id': resetuper.get_target_pillar_id(cid, args['hosts'][resetuper.fqdn]['subcid']),
        }
    )

    admin_api.default_headers = {'X-YaCloud-SubjectToken': iam.get_token()}
    backups = admin_api.get_backups('clickhouse', cid)

    shard_name = args['hosts'][fqdn]['shard_name']
    backup_info: Optional[dict] = None
    for backup in backups['backups']:
        if shard_id is None:
            backup_info = backup
            break
        if shard_name in backup['sourceShardNames']:
            backup_info = backup
            break

    if backup_info is None:
        raise ExposedException(f'No backup to restore host {fqdn}')

    backup_id = backup_info['id'].replace(f'{cid}:', '').replace(f'{shard_id}:', '')

    args.update(
        {
            'restore-from': {
                'cid': cid,
                's3-path': f'ch_backup/{cid}/{shard_name}/{backup_id}',
                'backup-id': backup_id,
            },
        }
    )
    return args


class ClickhousePortoReaddHost(PortoOnlineResetupHost):
    def get_create_executer(self) -> BaseCreateExecutor:
        args = args_for_host_create(self, self.cid, self.fqdn, self.metadb)
        if args['node_role'] == 'clickhouse':
            return ClickhouseHostCreateTask(self.config, self.task, self.queue, args)
        else:
            return ZookeeperHostCreateTask(self.config, self.task, self.queue, args)


class ClickhousePortoRestoreHost(PortoOnlineResetupHost):
    def get_create_executer(self) -> BaseCreateExecutor:
        args = self.get_create_host_args(self.cid, self.fqdn)
        self.task['task_type'] = RESTORE
        return ClusterRestoreTask(self.config, self.task, self.queue, args)

    def get_create_host_args(self, cid: str, fqdn: str) -> dict:
        admin_api = InternalApi(self.config, self.task, self.queue, api_url=self.get_admin_api_host())
        iam = IamJwt(self.config, self.task, self.queue)
        return args_for_cluster_create(self, cid, fqdn, admin_api=admin_api, iam=iam)


class ClickhouseComputeOnlineReaddHost(ComputeOnlineResetupHost):
    def get_create_executer(self) -> BaseCreateExecutor:
        args = args_for_host_create(self, self.cid, self.fqdn, self.metadb)
        if args['node_role'] == 'clickhouse':
            return ClickhouseHostCreateTask(self.config, self.task, self.queue, args)
        else:
            return ZookeeperHostCreateTask(self.config, self.task, self.queue, args)


class ClickhouseComputeOnlineRestoreHost(ComputeOnlineResetupHost):
    def get_create_executer(self) -> BaseCreateExecutor:
        args = self.get_create_host_args(self.cid, self.fqdn)
        self.task['task_type'] = RESTORE
        return ClusterRestoreTask(self.config, self.task, self.queue, args)

    def get_create_host_args(self, cid: str, fqdn: str) -> dict:
        admin_api = InternalApi(self.config, self.task, self.queue, api_url=self.get_admin_api_host())
        iam = IamJwt(self.config, self.task, self.queue)
        return args_for_cluster_create(self, cid, fqdn, admin_api=admin_api, iam=iam)


class ClickhouseComputeOfflineReaddHost(ComputeOfflineResetupHost):
    def get_create_executer(self) -> BaseCreateExecutor:
        if self.args['node_role'] == 'clickhouse':
            return ClickhouseHostCreateTask(self.config, self.task, self.queue, self.args)
        else:
            return ZookeeperHostCreateTask(self.config, self.task, self.queue, self.args)

    def get_args_for_create_task(self, cid: str, fqdn: str) -> dict:
        return args_for_host_create(self, cid, fqdn, self.metadb)

    def get_stop_task_class(self, args: dict) -> ClusterStopExecutor:
        return ClusterStopTask(self.config, self.task, self.queue, args)

    def get_start_task_class(self, args: dict) -> ClusterStartExecutor:
        return ClusterStartTask(self.config, self.task, self.queue, args)


class ClickhouseComputeOfflineRestoreHost(ComputeOfflineResetupHost):
    def get_create_executer(self) -> BaseCreateExecutor:
        self.task['task_type'] = RESTORE
        return ClusterRestoreTask(self.config, self.task, self.queue, self.args)

    def get_args_for_create_task(self, cid: str, fqdn: str) -> dict:
        admin_api = InternalApi(self.config, self.task, self.queue, api_url=self.get_admin_api_host())
        iam = IamJwt(self.config, self.task, self.queue)
        return args_for_cluster_create(self, cid, fqdn, admin_api=admin_api, iam=iam)

    def get_stop_task_class(self, args: dict) -> ClusterStopExecutor:
        return ClusterStopTask(self.config, self.task, self.queue, args)

    def get_start_task_class(self, args: dict) -> ClusterStartExecutor:
        return ClusterStartTask(self.config, self.task, self.queue, args)


class TaskType:
    clickhouse_online = 'clickhouse_cluster_online_resetup'
    clickhouse_offline = 'clickhouse_cluster_offline_resetup'


ROUTINE_MAP = {
    (EnvName.PORTO, ResetupActionType.READD, TaskType.clickhouse_online): ClickhousePortoReaddHost,
    (EnvName.PORTO, ResetupActionType.RESTORE, TaskType.clickhouse_online): ClickhousePortoRestoreHost,
    (EnvName.PORTO, ResetupActionType.READD, TaskType.clickhouse_offline): ClickhousePortoReaddHost,
    (EnvName.PORTO, ResetupActionType.RESTORE, TaskType.clickhouse_offline): ClickhousePortoRestoreHost,
    (EnvName.COMPUTE, ResetupActionType.READD, TaskType.clickhouse_online): ClickhouseComputeOnlineReaddHost,
    (EnvName.COMPUTE, ResetupActionType.RESTORE, TaskType.clickhouse_online): ClickhouseComputeOnlineRestoreHost,
    (EnvName.COMPUTE, ResetupActionType.READD, TaskType.clickhouse_offline): ClickhouseComputeOfflineReaddHost,
    (EnvName.COMPUTE, ResetupActionType.RESTORE, TaskType.clickhouse_offline): ClickhouseComputeOfflineRestoreHost,
}


@register_executor(TaskType.clickhouse_online)
@register_executor(TaskType.clickhouse_offline)
class ClickhouseClusterResetup(BaseExecutor):
    """
    Resetup clickhouse.
    """

    routine: Resetup

    def __init__(self, config, task, queue, task_args):
        super().__init__(config, task, queue, task_args)
        self.routine = init_routine(
            config=config,
            task=task,
            queue=queue,
            routine_class_map=ROUTINE_MAP,
            resetup_args=resetup_args_from_task_args(task_args),
            resetup_action=task_args['resetup_action'],
        )

    def run(self):
        self.routine.resetup_with_lock()
