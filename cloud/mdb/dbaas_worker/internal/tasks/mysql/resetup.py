"""
MySQL Resetup host executor
"""
from datetime import datetime

from cloud.mdb.dbaas_worker.internal.environment import EnvironmentName
from cloud.mdb.dbaas_worker.internal.exceptions import ExposedException
from cloud.mdb.dbaas_worker.internal.providers.base_metadb import BaseMetaDBProvider, execute
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
from .cluster.create import MySQLClusterCreate as ClusterCreateTask, RESTORE
from .cluster.start import MySQLClusterStart as ClusterStartTask
from .cluster.stop import MySQLClusterStop as ClusterStopTask
from .host.create import MySQLHostCreate as HostCreateTask
from ..common.cluster.start import ClusterStartExecutor
from ..common.cluster.stop import ClusterStopExecutor
from ..common.create import BaseCreateExecutor


def args_for_host_create(resetuper: Resetup, cid: str, fqdn: str, mysql_zk_hosts: list[str]) -> dict:
    args = resetuper.get_base_task_args(cid, resetuper.ignore_hosts)

    subcid = args['hosts'][fqdn]['subcid']
    shard_id = args['hosts'][fqdn]['shard_id']
    if len(args['hosts']) == 1:
        raise ExposedException('Adding hosts into cluster with one host will fail')

    args['host'] = fqdn
    args['subcid'] = subcid
    args['shard_id'] = shard_id
    args['zk_hosts'] = mysql_zk_hosts

    return args


def get_mysql_zk_hosts(cluster_id: str, metadb: BaseMetaDBProvider) -> list:
    """
    Get zk_hosts from cluster pillar for mysql
    """
    with metadb.get_master_conn() as conn:
        with conn:
            cur = conn.cursor()
            res = execute(
                cur,
                'get_pillar',
                path=['data', 'mysql', 'zk_hosts'],
                cid=cluster_id,
                subcid=None,
                fqdn=None,
                shard_id=None,
            )
            data = res[0]['value']
            return data


def args_for_cluster_create(resetuper: Resetup, cid: str, fqdn: str, mysql_zk_hosts: list[str]):

    args = resetuper.get_base_task_args(cid, resetuper.ignore_hosts)
    args['host'] = fqdn

    if len(args['hosts']) != 1:
        raise ExposedException('Restore hosts from cluster with more than one host will fail')

    args.update(
        {
            's3_bucket': resetuper.get_s3_bucket(cid),
            'target-pillar-id': resetuper.get_target_pillar_id(cid, args['hosts'][resetuper.fqdn]['subcid']),
        }
    )

    args.update(
        {
            'zk_hosts': mysql_zk_hosts,
            'restore-from': {
                'cid': cid,
                'time': f'{datetime.utcnow().isoformat()}+00:00',
                'backup-id': 'LATEST',
            },
        }
    )
    return args


class MysqlPortoRestoreHost(PortoOnlineResetupHost):
    def get_create_executer(self) -> BaseCreateExecutor:
        args = self.get_create_host_args(self.cid, self.fqdn)
        self.task['task_type'] = RESTORE
        return ClusterCreateTask(self.config, self.task, self.queue, args)

    def get_create_host_args(self, cid: str, fqdn: str) -> dict:
        return args_for_cluster_create(self, cid, fqdn, mysql_zk_hosts=get_mysql_zk_hosts(cid, self.metadb))


class MysqlPortoReaddHost(PortoOnlineResetupHost):
    def get_create_executer(self) -> BaseCreateExecutor:
        args = self.get_create_host_args(self.cid, self.fqdn)
        return HostCreateTask(self.config, self.task, self.queue, args)

    def get_create_host_args(self, cid: str, fqdn: str) -> dict:
        return args_for_host_create(self, cid, fqdn, mysql_zk_hosts=get_mysql_zk_hosts(cid, self.metadb))


class MysqlComputeOnlineReaddHost(ComputeOnlineResetupHost):
    def get_create_executer(self) -> BaseCreateExecutor:
        args = self.get_create_host_args(self.cid, self.fqdn)
        return HostCreateTask(self.config, self.task, self.queue, args)

    def get_create_host_args(self, cid: str, fqdn: str) -> dict:
        return args_for_host_create(self, cid, fqdn, mysql_zk_hosts=get_mysql_zk_hosts(cid, self.metadb))


class MysqlComputeOfflineReaddHost(ComputeOfflineResetupHost):
    def get_create_executer(self) -> BaseCreateExecutor:
        return HostCreateTask(self.config, self.task, self.queue, self.args)

    def get_args_for_create_task(self, cid: str, fqdn: str) -> dict:
        return args_for_host_create(self, cid, fqdn, mysql_zk_hosts=get_mysql_zk_hosts(cid, self.metadb))

    def get_stop_task_class(self, args: dict) -> ClusterStopExecutor:
        return ClusterStopTask(self.config, self.task, self.queue, args)

    def get_start_task_class(self, args: dict) -> ClusterStartExecutor:
        return ClusterStartTask(self.config, self.task, self.queue, args)


class MysqlComputeOfflineRestoreHost(ComputeOfflineResetupHost):
    def get_args_for_create_task(self, cid: str, fqdn: str) -> dict:
        return args_for_cluster_create(self, cid, fqdn, mysql_zk_hosts=get_mysql_zk_hosts(cid, self.metadb))

    def get_stop_task_class(self, args: dict) -> ClusterStopExecutor:
        return ClusterStopTask(self.config, self.task, self.queue, args)

    def get_start_task_class(self, args: dict) -> ClusterStartExecutor:
        return ClusterStartTask(self.config, self.task, self.queue, args)

    def get_create_executer(self) -> BaseCreateExecutor:
        self.task['task_type'] = RESTORE
        return ClusterCreateTask(self.config, self.task, self.queue, self.args)


class MysqlComputeOnlineRestoreHost(ComputeOnlineResetupHost):
    def get_create_executer(self) -> BaseCreateExecutor:
        args = self.get_create_host_args(self.cid, self.fqdn)
        self.task['task_type'] = RESTORE
        return ClusterCreateTask(self.config, self.task, self.queue, args)

    def get_create_host_args(self, cid: str, fqdn: str) -> dict:
        return args_for_cluster_create(self, cid, fqdn, mysql_zk_hosts=get_mysql_zk_hosts(cid, self.metadb))


class TaskType:
    mysql_online = 'mysql_cluster_online_resetup'
    mysql_offline = 'mysql_cluster_offline_resetup'


ROUTINE_MAP = {
    (EnvironmentName.PORTO, ResetupActionType.READD, TaskType.mysql_online): MysqlPortoReaddHost,
    (EnvironmentName.PORTO, ResetupActionType.RESTORE, TaskType.mysql_online): MysqlPortoRestoreHost,
    (EnvironmentName.PORTO, ResetupActionType.READD, TaskType.mysql_offline): MysqlPortoReaddHost,
    (EnvironmentName.PORTO, ResetupActionType.RESTORE, TaskType.mysql_offline): MysqlPortoRestoreHost,
    (EnvironmentName.COMPUTE, ResetupActionType.READD, TaskType.mysql_online): MysqlComputeOnlineReaddHost,
    (EnvironmentName.COMPUTE, ResetupActionType.RESTORE, TaskType.mysql_online): MysqlComputeOnlineRestoreHost,
    (
        EnvironmentName.COMPUTE,
        ResetupActionType.RESTORE,
        TaskType.mysql_offline,
    ): MysqlComputeOfflineRestoreHost,
    (EnvironmentName.COMPUTE, ResetupActionType.READD, TaskType.mysql_offline): MysqlComputeOfflineReaddHost,
}


@register_executor(TaskType.mysql_online)
@register_executor(TaskType.mysql_offline)
class MySQLClusterResetup(BaseExecutor):
    """
    Resetup mysql.
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
