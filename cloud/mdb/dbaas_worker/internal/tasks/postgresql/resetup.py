"""
PostgreSQL Resetup host executor
"""
from datetime import datetime

from cloud.mdb.dbaas_worker.internal.environment import get_env_name_from_config, EnvironmentName
from cloud.mdb.dbaas_worker.internal.exceptions import ExposedException
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
from .cluster.create import PostgreSQLClusterCreate as ClusterCreateTask
from .cluster.create import RESTORE
from .cluster.start import PostgreSQLClusterStart as ClusterStartTask
from .cluster.stop import PostgreSQLClusterStop as ClusterStopTask
from .host.create import PostgreSQLHostCreate as HostCreateTask
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


def args_for_cluster_create(resetuper: Resetup, cid: str, fqdn: str):
    args = resetuper.get_base_task_args(cid, resetuper.ignore_hosts)
    args['host'] = fqdn

    if len(args['hosts']) != 1:
        raise ExposedException('Restore hosts from cluster with more than one host will fail')

    args.update(
        {
            's3_bucket': resetuper.get_s3_bucket(cid),
            'target-pillar-id': resetuper.get_target_pillar_id(cid, args['hosts'][resetuper.fqdn]['subcid']),
            'restore-from': {
                'cid': cid,
                'time': f'{datetime.utcnow().isoformat()}+00:00',
                'backup-id': 'LATEST',
                'time-inclusive': True,
                'restore-latest': True,
                'rename-database-from-to': {},
            },
        }
    )
    return args


class PostgresqlPortoRestoreHost(PortoOnlineResetupHost):
    def get_create_executer(self) -> BaseCreateExecutor:
        args = self.get_create_host_args(self.cid, self.fqdn)
        self.task['task_type'] = RESTORE
        return ClusterCreateTask(self.config, self.task, self.queue, args)

    def get_create_host_args(self, cid: str, fqdn: str) -> dict:
        return args_for_cluster_create(self, cid, fqdn)


class PostgresqlPortoReaddHost(PortoOnlineResetupHost):
    def get_create_executer(self) -> BaseCreateExecutor:
        args = self.get_create_host_args(self.cid, self.fqdn)
        return HostCreateTask(self.config, self.task, self.queue, args)

    def get_create_host_args(self, cid: str, fqdn: str) -> dict:
        return args_for_host_create(self, cid, fqdn)


class PostgresqlComputeOnlineReaddHost(ComputeOnlineResetupHost):
    def get_create_executer(self) -> BaseCreateExecutor:
        args = self.get_create_host_args(self.cid, self.fqdn)
        return HostCreateTask(self.config, self.task, self.queue, args)

    def get_create_host_args(self, cid: str, fqdn: str) -> dict:
        return args_for_host_create(self, cid, fqdn)


class PostgresqlComputeOfflineReaddHost(ComputeOfflineResetupHost):
    def get_create_executer(self) -> BaseCreateExecutor:
        return HostCreateTask(self.config, self.task, self.queue, self.args)

    def get_args_for_create_task(self, cid: str, fqdn: str) -> dict:
        return args_for_host_create(self, cid, fqdn)

    def get_stop_task_class(self, args: dict) -> ClusterStopExecutor:
        return ClusterStopTask(self.config, self.task, self.queue, args)

    def get_start_task_class(self, args: dict) -> ClusterStartExecutor:
        return ClusterStartTask(self.config, self.task, self.queue, args)


class PostgresqlComputeOfflineRestoreHost(ComputeOfflineResetupHost):
    def get_create_executer(self) -> BaseCreateExecutor:
        self.task['task_type'] = RESTORE
        return ClusterCreateTask(self.config, self.task, self.queue, self.args)

    def get_args_for_create_task(self, cid: str, fqdn: str) -> dict:
        return args_for_cluster_create(self, cid, fqdn)

    def get_stop_task_class(self, args: dict) -> ClusterStopExecutor:
        return ClusterStopTask(self.config, self.task, self.queue, args)

    def get_start_task_class(self, args: dict) -> ClusterStartExecutor:
        return ClusterStartTask(self.config, self.task, self.queue, args)


class PostgresqlComputeOnlineRestoreHost(ComputeOnlineResetupHost):
    def get_create_executer(self) -> BaseCreateExecutor:
        args = self.get_create_host_args(self.cid, self.fqdn)
        self.task['task_type'] = RESTORE
        return ClusterCreateTask(self.config, self.task, self.queue, args)

    def get_create_host_args(self, cid: str, fqdn: str) -> dict:
        return args_for_cluster_create(self, cid, fqdn)


class TaskType:
    postgres_online = 'postgresql_cluster_online_resetup'
    postgres_offline = 'postgresql_cluster_offline_resetup'


ROUTINE_MAP = {
    (EnvironmentName.PORTO, ResetupActionType.READD, TaskType.postgres_online): PostgresqlPortoReaddHost,
    (EnvironmentName.PORTO, ResetupActionType.RESTORE, TaskType.postgres_online): PostgresqlPortoRestoreHost,
    (EnvironmentName.PORTO, ResetupActionType.READD, TaskType.postgres_offline): PostgresqlPortoReaddHost,
    (EnvironmentName.PORTO, ResetupActionType.RESTORE, TaskType.postgres_offline): PostgresqlPortoRestoreHost,
    (EnvironmentName.COMPUTE, ResetupActionType.READD, TaskType.postgres_online): PostgresqlComputeOnlineReaddHost,
    (EnvironmentName.COMPUTE, ResetupActionType.RESTORE, TaskType.postgres_online): PostgresqlComputeOnlineRestoreHost,
    (
        EnvironmentName.COMPUTE,
        ResetupActionType.RESTORE,
        TaskType.postgres_offline,
    ): PostgresqlComputeOfflineRestoreHost,
    (EnvironmentName.COMPUTE, ResetupActionType.READD, TaskType.postgres_offline): PostgresqlComputeOfflineReaddHost,
}


@register_executor(TaskType.postgres_online)
@register_executor(TaskType.postgres_offline)
class PostgreSQLClusterResetup(BaseExecutor):
    """
    Resetup postgresql host
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
        fqdn = self.args['fqdn']
        if get_env_name_from_config(self.config) == EnvironmentName.COMPUTE and fqdn.endswith(
            self.config.postgresql.managed_zone
        ):
            raise RuntimeError(f'Managed hostname {fqdn} used')

        self.routine.resetup_with_lock()
