"""
Redis Resetup host executor
"""
from typing import Optional

from cloud.mdb.dbaas_worker.internal.environment import EnvironmentName
from cloud.mdb.dbaas_worker.internal.exceptions import ExposedException
from cloud.mdb.dbaas_worker.internal.providers.iam_jwt import IamJwt
from cloud.mdb.dbaas_worker.internal.providers.internal_api import InternalApi
from cloud.mdb.dbaas_worker.internal.tasks.common.base import BaseExecutor
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
from .cluster.create import RedisClusterCreate as ClusterCreateTask, RESTORE
from .cluster.start import RedisClusterStart as ClusterStartTask
from .cluster.stop import RedisClusterStop as ClusterStopTask
from .host.create import RedisHostCreate as HostCreateTask
from ..common.cluster.start import ClusterStartExecutor
from ..common.cluster.stop import ClusterStopExecutor
from ..common.create import BaseCreateExecutor


def args_for_host_create(resetuper: Resetup, cid: str, fqdn: str) -> dict:
    args = resetuper.get_base_task_args(cid, resetuper.ignore_hosts)

    subcid = args['hosts'][fqdn]['subcid']
    shard_id = args['hosts'][fqdn]['shard_id']
    if len(args['hosts']) == 1:
        raise ExposedException('Adding hosts into cluster with one host will fail')

    args['host'] = fqdn
    args['subcid'] = subcid
    args['shard_id'] = shard_id

    return args


def args_for_cluster_create(resetuper: Resetup, cid: str, fqdn: str, admin_api: InternalApi, iam: IamJwt):

    args = resetuper.get_base_task_args(cid, resetuper.ignore_hosts)
    args['host'] = fqdn
    shard_id = args['hosts'][fqdn]['shard_id']

    if len([hostname for hostname, host_info in args['hosts'].items() if host_info['shard_id'] == shard_id]) != 1:
        raise ExposedException('Restoring shard with more than one host will fail')

    args.update(
        {
            's3_bucket': resetuper.get_s3_bucket(cid),
            'target-pillar-id': resetuper.get_target_pillar_id(cid, args['hosts'][resetuper.fqdn]['subcid']),
        }
    )

    admin_api.default_headers = {'X-YaCloud-SubjectToken': iam.get_token()}
    backups = admin_api.get_backups('redis', cid)

    shard_id = args['hosts'][fqdn]['shard_id']
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
                'backup-id': backup_id,
                's3-path': f'redis-backup/{cid}/{shard_id}',
            },
        }
    )
    return args


class RedisPortoRestoreHost(PortoOnlineResetupHost):
    def get_create_executer(self) -> BaseCreateExecutor:
        args = self.get_create_host_args(self.cid, self.fqdn)
        self.task['task_type'] = RESTORE
        return ClusterCreateTask(self.config, self.task, self.queue, args)

    def get_create_host_args(self, cid: str, fqdn: str) -> dict:
        iam = IamJwt(self.config, self.task, self.queue)
        admin_api = InternalApi(self.config, self.task, self.queue, api_url=self.get_admin_api_host())
        return args_for_cluster_create(self, cid, fqdn, admin_api=admin_api, iam=iam)


class RedisPortoReaddHost(PortoOnlineResetupHost):
    def get_create_executer(self) -> BaseCreateExecutor:
        args = self.get_create_host_args(self.cid, self.fqdn)
        return HostCreateTask(self.config, self.task, self.queue, args)

    def get_create_host_args(self, cid: str, fqdn: str) -> dict:
        return args_for_host_create(self, cid, fqdn)


class RedisComputeOnlineReaddHost(ComputeOnlineResetupHost):
    def get_create_executer(self) -> BaseCreateExecutor:
        args = self.get_create_host_args(self.cid, self.fqdn)
        return HostCreateTask(self.config, self.task, self.queue, args)

    def get_create_host_args(self, cid: str, fqdn: str) -> dict:
        return args_for_host_create(self, cid, fqdn)


class RedisComputeOfflineReaddHost(ComputeOfflineResetupHost):
    def get_create_executer(self) -> BaseCreateExecutor:
        return HostCreateTask(self.config, self.task, self.queue, self.args)

    def get_args_for_create_task(self, cid: str, fqdn: str) -> dict:
        return args_for_host_create(self, cid, fqdn)

    def get_stop_task_class(self, args: dict) -> ClusterStopExecutor:
        return ClusterStopTask(self.config, self.task, self.queue, args)

    def get_start_task_class(self, args: dict) -> ClusterStartExecutor:
        return ClusterStartTask(self.config, self.task, self.queue, args)


class RedisComputeOfflineRestoreHost(ComputeOfflineResetupHost):
    def get_create_executer(self) -> BaseCreateExecutor:
        self.task['task_type'] = RESTORE
        return ClusterCreateTask(self.config, self.task, self.queue, self.args)

    def get_args_for_create_task(self, cid: str, fqdn: str) -> dict:
        iam = IamJwt(self.config, self.task, self.queue)
        admin_api = InternalApi(self.config, self.task, self.queue, api_url=self.get_admin_api_host())
        return args_for_cluster_create(self, cid, fqdn, admin_api=admin_api, iam=iam)

    def get_stop_task_class(self, args: dict) -> ClusterStopExecutor:
        return ClusterStopTask(self.config, self.task, self.queue, args)

    def get_start_task_class(self, args: dict) -> ClusterStartExecutor:
        return ClusterStartTask(self.config, self.task, self.queue, args)


class RedisComputeOnlineRestoreHost(ComputeOnlineResetupHost):
    def get_create_executer(self) -> BaseCreateExecutor:
        args = self.get_create_host_args(self.cid, self.fqdn)
        self.task['task_type'] = RESTORE
        return ClusterCreateTask(self.config, self.task, self.queue, args)

    def get_create_host_args(self, cid: str, fqdn: str) -> dict:
        iam = IamJwt(self.config, self.task, self.queue)
        admin_api = InternalApi(self.config, self.task, self.queue, api_url=self.get_admin_api_host())
        return args_for_cluster_create(self, cid, fqdn, admin_api=admin_api, iam=iam)


class TaskType:
    redis_online = 'redis_cluster_online_resetup'
    redis_offline = 'redis_cluster_offline_resetup'


ROUTINE_MAP = {
    (EnvironmentName.PORTO, ResetupActionType.READD, TaskType.redis_online): RedisPortoReaddHost,
    (EnvironmentName.PORTO, ResetupActionType.RESTORE, TaskType.redis_online): RedisPortoRestoreHost,
    (EnvironmentName.PORTO, ResetupActionType.READD, TaskType.redis_offline): RedisPortoReaddHost,
    (EnvironmentName.PORTO, ResetupActionType.RESTORE, TaskType.redis_offline): RedisPortoRestoreHost,
    (EnvironmentName.COMPUTE, ResetupActionType.READD, TaskType.redis_online): RedisComputeOnlineReaddHost,
    (EnvironmentName.COMPUTE, ResetupActionType.RESTORE, TaskType.redis_online): RedisComputeOnlineRestoreHost,
    (
        EnvironmentName.COMPUTE,
        ResetupActionType.RESTORE,
        TaskType.redis_offline,
    ): RedisComputeOfflineRestoreHost,
    (EnvironmentName.COMPUTE, ResetupActionType.READD, TaskType.redis_offline): RedisComputeOfflineReaddHost,
}


@register_executor(TaskType.redis_online)
@register_executor(TaskType.redis_offline)
class RedisClusterResetup(BaseExecutor):
    """
    Resetup redis host
    """

    routine: Resetup

    def __init__(self, config, task, queue, task_args):
        super().__init__(config, task, queue, task_args)
        self.resetup_args = resetup_args_from_task_args(task_args)
        self.routine = init_routine(
            config=config,
            task=task,
            queue=queue,
            routine_class_map=ROUTINE_MAP,
            resetup_args=self.resetup_args,
            resetup_action=task_args['resetup_action'],
        )

    def run(self):
        self.routine.resetup_with_lock()
