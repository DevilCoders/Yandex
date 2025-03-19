"""
Greenplum Resetup host executor
"""

from ....internal.types import Task
from ....internal.environment import EnvironmentName
from ....internal.exceptions import ExposedException
from ....internal.tasks.common.base import BaseExecutor
from ....internal.tasks.common.resetup_common import (
    PortoOnlineResetupHost,
    ResetupActionType,
    ComputeOnlineResetupHost,
    ComputeOfflineResetupHost,
    Resetup,
    init_routine,
    resetup_args_from_task_args,
    ResetupArgs,
)
from queue import Queue

from ....internal.tasks.utils import register_executor

from ..common.cluster.start import ClusterStartExecutor
from ..common.cluster.stop import ClusterStopExecutor

from .cluster.start import GreenplumClusterStart as ClusterStartTask
from .cluster.stop import GreenplumClusterStop as ClusterStopTask

from ..common.create import BaseCreateExecutor

from .host.resetup_segment_host import GreenplumSegmentHostReSetup
from .host.resetup_master_host import GreenplumMasterHostReSetup

from .utils import (
    SEGMENT_ROLE_TYPE,
    MASTER_ROLE_TYPE,
    classify_host_map,
)


def args_for_host_create(resetuper: Resetup, cid: str, fqdn: str) -> dict:
    args = resetuper.get_base_task_args(cid, resetuper.ignore_hosts)

    subcid = args['hosts'][fqdn]['subcid']

    args['host'] = fqdn
    args['subcid'] = subcid

    return args


class GreenplumPortoReaddHost(PortoOnlineResetupHost):
    def __init__(self, config, task: Task, queue: Queue, resetup_args: ResetupArgs):
        super().__init__(config, task, queue, resetup_args)

        args = self.get_create_host_args(self.cid, self.fqdn)
        master_hosts, segment_hosts = classify_host_map(args['hosts'])
        role = args['hosts'][args['host']]['roles'][0]
        if role == MASTER_ROLE_TYPE and len(master_hosts) == 1:
            raise ExposedException('Resetup master host into cluster with one host will fail')

    def get_create_executer(self) -> BaseCreateExecutor:
        args = self.get_create_host_args(self.cid, self.fqdn)
        role = args['hosts'][args['host']]['roles'][0]
        if role == SEGMENT_ROLE_TYPE:
            return GreenplumSegmentHostReSetup(self.config, self.task, self.queue, args)

        return GreenplumMasterHostReSetup(self.config, self.task, self.queue, args)

    def get_create_host_args(self, cid: str, fqdn: str) -> dict:
        return args_for_host_create(self, cid, fqdn)

    def before_resetup(self):
        args = self.get_create_host_args(self.cid, self.fqdn)
        role = args["hosts"][args["host"]]["roles"][0]
        if role == SEGMENT_ROLE_TYPE:
            args = self.get_create_host_args(self.cid, self.fqdn)
            execute = GreenplumSegmentHostReSetup(self.config, self.task, self.queue, args)
            execute.before_create_host()

    def after_resetup(self):
        args = self.get_create_host_args(self.cid, self.fqdn)
        role = args["hosts"][args["host"]]["roles"][0]
        if role == SEGMENT_ROLE_TYPE:
            args = self.get_create_host_args(self.cid, self.fqdn)
            execute = GreenplumSegmentHostReSetup(self.config, self.task, self.queue, args)
            execute.after_create_host()

    def _resetup(self):
        self.before_resetup()
        self.drop_deploy_data(self.fqdn)

        if not self.preserve:
            container = self.dbm.get_container(self.fqdn)
            if container.get("dom0") in (self.resetup_from or None, None):
                self.prepare_porto_container(self.fqdn)

        self._recreate_leg()
        self.after_resetup()


class GreenplumComputeOnlineReaddHost(ComputeOnlineResetupHost):
    def __init__(self, config, task: Task, queue: Queue, resetup_args: ResetupArgs):
        super().__init__(config, task, queue, resetup_args)

        args = self.get_create_host_args(self.cid, self.fqdn)
        master_hosts, segment_hosts = classify_host_map(args['hosts'])
        role = args['hosts'][args['host']]['roles'][0]
        if role == MASTER_ROLE_TYPE and len(master_hosts) == 1:
            raise ExposedException('Resetup master host into cluster with one host will fail')

    def get_create_executer(self) -> BaseCreateExecutor:
        args = self.get_create_host_args(self.cid, self.fqdn)
        role = args['hosts'][args['host']]['roles'][0]
        if role == SEGMENT_ROLE_TYPE:
            return GreenplumSegmentHostReSetup(self.config, self.task, self.queue, args)

        return GreenplumMasterHostReSetup(self.config, self.task, self.queue, args)

    def get_create_host_args(self, cid: str, fqdn: str) -> dict:
        return args_for_host_create(self, cid, fqdn)

    def before_resetup(self):
        args = self.get_create_host_args(self.cid, self.fqdn)
        role = args["hosts"][args["host"]]["roles"][0]
        if role == SEGMENT_ROLE_TYPE:
            args = self.get_create_host_args(self.cid, self.fqdn)
            execute = GreenplumSegmentHostReSetup(self.config, self.task, self.queue, args)
            execute.before_create_host()

    def after_resetup(self):
        args = self.get_create_host_args(self.cid, self.fqdn)
        role = args["hosts"][args["host"]]["roles"][0]
        if role == SEGMENT_ROLE_TYPE:
            args = self.get_create_host_args(self.cid, self.fqdn)
            execute = GreenplumSegmentHostReSetup(self.config, self.task, self.queue, args)
            execute.after_create_host()

    def _resetup(self):
        fqdn = self.fqdn
        self.save_disks_if_needed()
        self.before_resetup()
        self.drop_deploy_data(fqdn)
        self.drop_instance(fqdn)
        self._recreate_leg()
        self.after_resetup()


class GreenplumComputeOfflineReaddHost(ComputeOfflineResetupHost):
    def __init__(self, config, task: Task, queue: Queue, resetup_args: ResetupArgs):
        super().__init__(config, task, queue, resetup_args)

        args = self.get_args_for_create_task(self.cid, self.fqdn)
        master_hosts, segment_hosts = classify_host_map(args['hosts'])
        role = args['hosts'][args['host']]['roles'][0]
        if role == MASTER_ROLE_TYPE and len(master_hosts) == 1:
            raise ExposedException('Resetup master host into cluster with one host will fail')

    def get_create_executer(self) -> BaseCreateExecutor:
        role = self.args['hosts'][self.args['host']]['roles'][0]
        if role == SEGMENT_ROLE_TYPE:
            return GreenplumSegmentHostReSetup(self.config, self.task, self.queue, self.args)

        return GreenplumMasterHostReSetup(self.config, self.task, self.queue, self.args)

    def get_args_for_create_task(self, cid: str, fqdn: str) -> dict:
        return args_for_host_create(self, cid, fqdn)

    def get_stop_task_class(self, args: dict) -> ClusterStopExecutor:
        return ClusterStopTask(self.config, self.task, self.queue, args)

    def get_start_task_class(self, args: dict) -> ClusterStartExecutor:
        return ClusterStartTask(self.config, self.task, self.queue, args)


class TaskType:
    greenplum_online = 'greenplum_cluster_online_resetup'
    greenplum_offline = 'greenplum_cluster_offline_resetup'


ROUTINE_MAP = {
    (EnvironmentName.PORTO, ResetupActionType.READD, TaskType.greenplum_online): GreenplumPortoReaddHost,
    (EnvironmentName.PORTO, ResetupActionType.READD, TaskType.greenplum_offline): GreenplumPortoReaddHost,
    (EnvironmentName.COMPUTE, ResetupActionType.READD, TaskType.greenplum_online): GreenplumComputeOnlineReaddHost,
    (EnvironmentName.COMPUTE, ResetupActionType.READD, TaskType.greenplum_offline): GreenplumComputeOfflineReaddHost,
}


@register_executor(TaskType.greenplum_online)
@register_executor(TaskType.greenplum_offline)
class GreenplumClusterResetup(BaseExecutor):
    """
    Resetup greenplum host
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
