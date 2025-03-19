"""
Kafka Resetup host executor
"""
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
from .cluster.start import KafkaClusterStart as ClusterStartTask
from .cluster.stop import KafkaClusterStop as ClusterStopTask
from .host.create import KafkaHostCreate as HostCreateTask
from .host.create import KafkaZookeeperHostCreate
from ..common.cluster.start import ClusterStartExecutor
from ..common.cluster.stop import ClusterStopExecutor
from ..common.create import BaseCreateExecutor


def args_for_host_create(resetuper: Resetup, cid: str, fqdn: str) -> dict:
    args = resetuper.get_base_task_args(cid, resetuper.ignore_hosts)

    subcid = args['hosts'][fqdn]['subcid']
    shard_id = args['hosts'][fqdn]['shard_id']
    if len(args['hosts']) == 1:
        raise ExposedException('Adding hosts into cluster with one host will fail')
    zk_hosts = [x for x in args['hosts'] if 'zk' in args['hosts'][x]['roles']]

    args['host'] = fqdn
    args['subcid'] = subcid
    args['shard_id'] = shard_id
    args['node_role'] = 'zk' if fqdn in zk_hosts else 'kafka'

    return args


def get_zookeeper_id(metadb: BaseMetaDBProvider, cid: str, fqdn: str) -> str:
    """
    Get zookeeper id for host in cluster
    """
    with metadb.get_master_conn() as conn:
        with conn:
            cur = conn.cursor()
            res = execute(
                cur, 'get_pillar', path=['data', 'zk', 'nodes'], cid=cid, subcid=None, fqdn=None, shard_id=None
            )
            return res[0]['value'][fqdn]


class KafkaPortoReaddHost(PortoOnlineResetupHost):
    def get_create_executer(self) -> BaseCreateExecutor:
        args = self.get_create_host_args(self.cid, self.fqdn)
        if args['node_role'] == 'kafka':
            return HostCreateTask(self.config, self.task, self.queue, args)
        else:
            return KafkaZookeeperHostCreate(self.config, self.task, self.queue, args)

    def get_create_host_args(self, cid: str, fqdn: str) -> dict:
        return args_for_host_create(self, cid, fqdn)


class KafkaComputeOnlineReaddHost(ComputeOnlineResetupHost):
    def get_create_executer(self) -> BaseCreateExecutor:
        args = self.get_create_host_args(self.cid, self.fqdn)
        if args['node_role'] == 'kafka':
            return HostCreateTask(self.config, self.task, self.queue, args)
        else:
            return KafkaZookeeperHostCreate(self.config, self.task, self.queue, args)

    def get_create_host_args(self, cid: str, fqdn: str) -> dict:
        return args_for_host_create(self, cid, fqdn)


class KafkaComputeOfflineReaddHost(ComputeOfflineResetupHost):
    def get_create_executer(self) -> BaseCreateExecutor:
        if self.args['node_role'] == 'kafka':
            return HostCreateTask(self.config, self.task, self.queue, self.args)
        else:
            return KafkaZookeeperHostCreate(self.config, self.task, self.queue, self.args)

    def get_args_for_create_task(self, cid: str, fqdn: str) -> dict:
        return args_for_host_create(self, cid, fqdn)

    def get_stop_task_class(self, args: dict) -> ClusterStopExecutor:
        return ClusterStopTask(self.config, self.task, self.queue, args)

    def get_start_task_class(self, args: dict) -> ClusterStartExecutor:
        return ClusterStartTask(self.config, self.task, self.queue, args)


class TaskType:
    kafka_online = 'kafka_cluster_online_resetup'
    kafka_offline = 'kafka_cluster_offline_resetup'


ROUTINE_MAP = {
    (EnvironmentName.PORTO, ResetupActionType.READD, TaskType.kafka_online): KafkaPortoReaddHost,
    (EnvironmentName.PORTO, ResetupActionType.READD, TaskType.kafka_offline): KafkaPortoReaddHost,
    (EnvironmentName.COMPUTE, ResetupActionType.READD, TaskType.kafka_online): KafkaComputeOnlineReaddHost,
    (EnvironmentName.COMPUTE, ResetupActionType.READD, TaskType.kafka_offline): KafkaComputeOfflineReaddHost,
}


@register_executor(TaskType.kafka_online)
@register_executor(TaskType.kafka_offline)
class KafkaClusterResetup(BaseExecutor):
    """
    Resetup kafka host
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
