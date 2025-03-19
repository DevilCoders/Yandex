"""
Elasticsearh Resetup host executor
"""
from cloud.mdb.dbaas_worker.internal.environment import EnvironmentName
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
from .cluster.create import ElasticsearchClusterCreate as ClusterCreateTask
from .cluster.start import ElasticsearchClusterStart as ClusterStartTask
from .cluster.stop import ElasticsearchClusterStop as ClusterStopTask
from .host.create import ElasticsearchHostCreate as HostCreateTask
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


class ElasticPortoReaddHost(PortoOnlineResetupHost):
    def get_create_executer(self) -> BaseCreateExecutor:
        args = self.get_create_host_args(self.cid, self.fqdn)
        return HostCreateTask(self.config, self.task, self.queue, args)

    def get_create_host_args(self, cid: str, fqdn: str) -> dict:
        return args_for_host_create(self, cid, fqdn)


class ElasticComputeOnlineReaddHost(ComputeOnlineResetupHost):
    def get_create_executer(self) -> BaseCreateExecutor:
        args = self.get_create_host_args(self.cid, self.fqdn)
        return ClusterCreateTask(self.config, self.task, self.queue, args)

    def get_create_host_args(self, cid: str, fqdn: str) -> dict:
        return args_for_host_create(self, cid, fqdn)


class ElasticComputeOfflineReaddHost(ComputeOfflineResetupHost):
    def get_create_executer(self) -> BaseCreateExecutor:
        return ClusterCreateTask(self.config, self.task, self.queue, self.args)

    def get_args_for_create_task(self, cid: str, fqdn: str) -> dict:
        return args_for_host_create(self, cid, fqdn)

    def get_stop_task_class(self, args: dict) -> ClusterStopExecutor:
        return ClusterStopTask(self.config, self.task, self.queue, args)

    def get_start_task_class(self, args: dict) -> ClusterStartExecutor:
        return ClusterStartTask(self.config, self.task, self.queue, args)


class TaskType:
    elastic_online = 'elasticsearch_cluster_online_resetup'
    elastic_offline = 'elasticsearch_cluster_offline_resetup'


ROUTINE_MAP = {
    (EnvironmentName.PORTO, ResetupActionType.READD, TaskType.elastic_online): ElasticPortoReaddHost,
    (EnvironmentName.PORTO, ResetupActionType.READD, TaskType.elastic_offline): ElasticPortoReaddHost,
    (EnvironmentName.COMPUTE, ResetupActionType.READD, TaskType.elastic_online): ElasticComputeOnlineReaddHost,
    (EnvironmentName.COMPUTE, ResetupActionType.READD, TaskType.elastic_offline): ElasticComputeOfflineReaddHost,
}


@register_executor(TaskType.elastic_online)
@register_executor(TaskType.elastic_offline)
class ElasticsearchClusterResetup(BaseExecutor):
    """
    Elasticsearch redis host
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
