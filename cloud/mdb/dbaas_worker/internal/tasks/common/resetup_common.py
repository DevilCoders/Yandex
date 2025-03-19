"""
Host resetup tool
"""

import json
import logging
import string
import time
from abc import ABC, abstractmethod
from copy import deepcopy
from enum import Enum
from queue import Queue
from secrets import choice
from typing import Any, Dict, List, NamedTuple, Optional

from dbaas_common.dict import combine_dict

from cloud.mdb.dbaas_worker.internal.environment import get_env_name_from_config
from cloud.mdb.dbaas_worker.internal.exceptions import ExposedException
from cloud.mdb.dbaas_worker.internal.logs import get_task_prefixed_logger
from cloud.mdb.dbaas_worker.internal.providers.base_metadb import BaseMetaDBProvider
from cloud.mdb.dbaas_worker.internal.providers.common import Change, TaskUpdate
from cloud.mdb.dbaas_worker.internal.providers.compute import ComputeApi, ComputeApiError
from cloud.mdb.dbaas_worker.internal.providers.conductor import ConductorApi, ConductorError
from cloud.mdb.dbaas_worker.internal.providers.dbm import DBMApi, DBMChange
from cloud.mdb.dbaas_worker.internal.providers.deploy import DeployAPI, deploy_dataplane_host
from cloud.mdb.dbaas_worker.internal.providers.mlock import Mlock
from cloud.mdb.dbaas_worker.internal.query import execute
from cloud.mdb.dbaas_worker.internal.tasks.common.base import SKIP_LOCK_CREATION_ARG
from cloud.mdb.dbaas_worker.internal.tasks.common.cluster.start import ClusterStartExecutor
from cloud.mdb.dbaas_worker.internal.tasks.common.cluster.stop import ClusterStopExecutor
from cloud.mdb.dbaas_worker.internal.tasks.common.create import BaseCreateExecutor
from cloud.mdb.dbaas_worker.internal.types import Task

log_stage = logging.getLogger('stages')


class ResetupActionType(Enum):
    RESTORE = 'restore'
    READD = 'readd'


class ResetupArgs(NamedTuple):
    fqdn: str
    cid: str
    preserve_if_possible: bool
    ignore_hosts: set
    lock_taken: bool
    try_save_disks: bool
    resetup_from: str


class Resetup(ABC):
    def __init__(self, config, task: Task, queue: Queue, resetup_args: ResetupArgs):
        self.config = config
        self.task = task
        task['cid'] = resetup_args.cid
        self.queue = queue
        self.logger = get_task_prefixed_logger(task, __name__)
        self.deploy_api = DeployAPI(config, task, queue)
        self.metadb = BaseMetaDBProvider(config, task, queue)
        self._resolver_cache: Optional[List[Dict[str, Any]]] = None
        self.fqdn = resetup_args.fqdn
        self.lock_taken_outside = resetup_args.lock_taken
        self.ignore_hosts = resetup_args.ignore_hosts
        self.cid = resetup_args.cid
        self.mlock = Mlock(config, task, queue)
        self.__stage_logger = log_stage
        self.resetup_from = resetup_args.resetup_from
        self.args: Dict[str, Any] = {}

    def stage_log(self, msg):
        self.__stage_logger.info(msg)

    def resetup_with_lock(self):
        try:
            self.stage_log('take lock')
            self._lock_if_needed()
            self._resetup()
            self.stage_log('resetup OK')
        except Exception as exc:
            logging.exception('Uncaught exception %s', exc)
            raise
        except KeyboardInterrupt:
            logging.info('interrupted by keyboard')
            return
        finally:
            self.stage_log('unlock')
            self._unlock_if_needed()
            self.stage_log('finished')

    def add_change(self, change):
        """
        Save change
        """
        self.task['changes'].append(change)
        self.task['context'].update(change.context)
        if change.context:
            self.logger.debug('Saving context: %s', change.context)
        serialized = change.serialize()
        self.queue.put(TaskUpdate(change.context, serialized))

    def context_get(self, key):
        """
        Task context get helper
        """
        value = self.task['context'].get(key)
        self.logger.debug('Getting %s from context: %s', key, value)
        return deepcopy(value)

    @abstractmethod
    def _resetup(self):
        pass

    def _lock_if_needed(self):
        if not self.lock_taken_outside:
            args = self.get_base_task_args(self.cid, ignore=set())
            self.mlock.lock_cluster(args['hosts'])

    def _unlock_if_needed(self):
        if not self.lock_taken_outside:
            self.mlock.unlock_cluster()

    def get_s3_bucket(self, cluster_id):
        """
        Get s3 bucket from cluster pillar
        """
        with self.metadb.get_master_conn() as conn:
            with conn:
                cur = conn.cursor()
                res = execute(
                    cur, 'get_pillar', path=['data', 's3_bucket'], cid=cluster_id, subcid=None, fqdn=None, shard_id=None
                )
                return res[0]['value']

    def get_target_pillar_id(self, cluster_id, subcid):
        """
        Insert target pillar and return id
        """
        target_pillar_id = ''.join([choice(string.ascii_letters + string.digits) for _ in range(16)])

        with self.metadb.get_master_conn() as conn:
            with conn:
                cur = conn.cursor()
                cluster_res = execute(
                    cur, 'get_pillar', path=['data'], cid=cluster_id, subcid=None, fqdn=None, shard_id=None
                )
                cluster_data = cluster_res[0]['value']

                subcid_res = execute(
                    cur, 'get_pillar', path=['data'], cid=None, subcid=subcid, fqdn=None, shard_id=None
                )
                if subcid_res:
                    data = combine_dict(cluster_data, subcid_res[0]['value'])
                else:
                    data = cluster_data

                execute(
                    cur,
                    'add_cluster_target_pillar',
                    fetch=False,
                    cid=cluster_id,
                    target_id=target_pillar_id,
                    value=json.dumps({'data': {'restore-from-pillar-data': data}}),
                )

        return target_pillar_id

    @abstractmethod
    def get_create_executer(self) -> BaseCreateExecutor:
        pass

    def drop_deploy_data(self, fqdn):
        """
        Drop deploy v1/v2 data for minion
        """
        if self.deploy_api.has_minion(fqdn):
            self.deploy_api.unregister_minion(fqdn)
        else:
            self.deploy_api.create_minion(fqdn, self.config.deploy.group)

    def resolve_cluster(self, cluster_id: str) -> List[Dict[str, Any]]:
        if self._resolver_cache is None:
            with self.metadb.get_master_conn() as conn:
                with conn:
                    cur = conn.cursor()
                    self._resolver_cache = execute(cur, 'generic_resolve', cid=cluster_id)

        return self._resolver_cache

    def get_base_task_args(self, cluster_id: str, ignore: set) -> dict:
        """
        Get common task args (e.g. hosts)
        """
        args = {'hosts': {}, 'cid': cluster_id, SKIP_LOCK_CREATION_ARG: True}
        res = self.resolve_cluster(cluster_id)
        for row in res:
            if ignore and row['fqdn'] in ignore:
                continue
            # Just silent mypy, don't want to dig why it complains:
            # common/resetup_common.py:199: error: Unsupported target for indexed assignment ("object")
            args['hosts'][row['fqdn']] = {x: row[x] for x in row if x not in ['name', 'fqdn']}  # type: ignore

        return args


def _save_disks(fqdn: str, compute_api: ComputeApi, logger) -> list:
    """
    Mark instance disks undeletable if possible
    """
    log_stage.info('saving disks')
    result = []
    try:
        instance = compute_api.get_instance(fqdn)
        if not instance:
            raise ExposedException(f'Unable to find instance {fqdn}')

        operation = compute_api.instance_stopped(instance.fqdn, instance.id, context_suffix='stop-to-save-disks')
        if operation:
            compute_api.operation_wait(operation)

        operation = compute_api.set_autodelete_for_boot_disk(instance.id, instance.boot_disk.disk_id, False)
        result.append(instance.boot_disk.disk_id)
        logger.info('Saved boot disk: %s', instance.boot_disk.disk_id)
        if operation:
            compute_api.operation_wait(operation)

        for disk_id in {x.disk_id for x in instance.secondary_disks}:
            operation = compute_api.detach_disk(instance.id, disk_id)
            logger.info('Saved secondary disk: %s', disk_id)
            if operation:
                compute_api.operation_wait(operation)
    except ComputeApiError as exc:
        logger.error(f'Unable to save instance disks: {exc!r}')
    return result


def get_admin_api_host(conductor: ConductorApi, admin_api_conductor_group: str) -> str:
    try:
        hosts = conductor.group_to_hosts(admin_api_conductor_group)
    except ConductorError as exc:
        raise ExposedException('admin api host is not available in conductor: {}'.format(str(exc)))
    if len(hosts) == 0:
        raise ExposedException('add some admin api hosts to "{}"?'.format(admin_api_conductor_group))
    return 'https://' + hosts[0]


class ComputeResetup(Resetup):
    def __init__(self, config, task: Task, queue: Queue, resetup_args: ResetupArgs):
        super().__init__(config, task, queue, resetup_args)
        self.try_save_disks = resetup_args.try_save_disks
        self.compute = ComputeApi(config, task, queue)
        self.conductor = ConductorApi(config, task, queue)

    def save_disks_if_needed(self):
        context_key = 'save_disks_if_needed'
        was_done = self.context_get(context_key)
        if not was_done:
            self.stage_log('save disks')
            if self.try_save_disks:
                _save_disks(self.fqdn, self.compute, self.logger)
            was_done = True
        else:
            self.stage_log('save disks skipped')
        self.add_change(
            Change(
                context_key,
                'finished',
                context={context_key: was_done},
                rollback=Change.noop_rollback,
            )
        )


class ComputeOnlineResetupHost(ComputeResetup):
    def __init__(self, config, task: Task, queue: Queue, resetup_args: ResetupArgs):
        super().__init__(config, task, queue, resetup_args)

    def get_admin_api_host(self):
        return get_admin_api_host(self.conductor, self.config.main.admin_api_conductor_group)

    def drop_instance(self, fqdn):
        """
        Delete instance
        """
        self.stage_log('drop instance')
        self.compute.operation_wait(
            self.compute.instance_absent(fqdn, instance_id=self.resetup_from or None), timeout=50 * 60
        )

    def _recreate_leg(self):
        """
        Create leg again.
        """
        self.stage_log('create leg')
        create_executer = self.get_create_executer()
        create_executer.run()

    def _resetup(self):
        self.save_disks_if_needed()
        self.drop_deploy_data(self.fqdn)
        self.drop_instance(self.fqdn)
        self._recreate_leg()


class ComputeOfflineResetupHost(ComputeResetup):
    def get_admin_api_host(self):
        return get_admin_api_host(self.conductor, self.config.main.admin_api_conductor_group)

    def _recreate_leg(self):
        """
        Create leg again.
        """
        create_executer = self.get_create_executer()
        create_executer.run()

    def _resetup(self):
        self.args = self.get_args_for_create_task(self.cid, self.fqdn)
        self.save_disks_if_needed()

        self.prepare_offline_cluster(self.args, self.fqdn)

        self.drop_deploy_data(self.fqdn)

        self.drop_instance(self.fqdn)

        self._recreate_leg()

        self.shutdown_offline_cluster(self.args)

        if self.try_save_disks:
            pass  # autodelete

    @abstractmethod
    def get_stop_task_class(self, args: dict) -> ClusterStopExecutor:
        """
        Stop cluster.

        It was stopped initially.
        """
        pass

    @abstractmethod
    def get_start_task_class(self, args: dict) -> ClusterStartExecutor:
        """
        Start cluster.

        With mongodb, zookeeper, you need access to other online cluster hosts.
        """
        pass

    def drop_instance(self, fqdn):
        """
        Delete instance
        """
        self.stage_log('drop instance')
        self.compute.operation_wait(
            self.compute.instance_absent(fqdn, instance_id=self.resetup_from or None), timeout=50 * 60
        )

    def prepare_offline_cluster(self, args, fqdn):
        """
        Start non-target instances with disabled billing
        """
        args['create_offline'] = True
        self.stage_log('disabling billing')
        start_args = deepcopy(args)
        disable_operations = []
        hosts = [x for x in args['hosts'] if x != fqdn]
        for host in hosts:
            operation = self.compute.disable_billing(host)
            disable_operations.append(operation)

        for i in disable_operations:
            self.compute.operation_wait(i)

        del start_args['hosts'][fqdn]
        if start_args['hosts']:
            task_class = self.get_start_task_class(start_args)
            task_class.run()

    def shutdown_offline_cluster(self, args):
        """
        Run stop cluster operation and enable billing on instances
        """
        shutdown_task = self.get_stop_task_class(args)
        shutdown_task.run()

        enable_operations = []
        for host in args['hosts']:
            operation_id = self.compute.enable_billing(host)
            if operation_id:
                enable_operations.append(operation_id)

        for i in enable_operations:
            self.compute.operation_wait(i)

    @abstractmethod
    def get_args_for_create_task(self, cid: str, fqdn: str) -> dict:
        pass


class PortoOnlineResetupHost(Resetup):
    def __init__(self, config, task: Task, queue: Queue, resetup_args: ResetupArgs):
        super().__init__(config, task, queue, resetup_args)
        self.dbm = DBMApi(config, task, queue)
        self.conductor = ConductorApi(config, task, queue)
        self.preserve = resetup_args.preserve_if_possible

    def get_admin_api_host(self):
        return get_admin_api_host(self.conductor, self.config.main.admin_api_conductor_group)

    def prepare_porto_container(self, fqdn):
        """
        Recreate porto container on new dom0
        """
        self.stage_log('Recreate porto container on new dom0')
        context_key = 'porto_container_transfered_from_dom0'
        transfer_id = self.context_get(context_key)
        if not transfer_id:
            change = self.dbm.update_container(fqdn, data=dict(dom0=None))

            if change.transfer is None:
                raise RuntimeError(f'Unable to init transfer. DBM change: {change}')

            transfer_id = change.transfer['id']
            self.add_change(
                Change(
                    context_key,
                    'initiated',
                    context={context_key: transfer_id},
                    rollback=Change.noop_rollback,
                )
            )
        context_key = 'porto_container_finish_transfer'
        transfer_finished = self.context_get(context_key)
        if not transfer_finished:
            self.dbm.finish_transfer(transfer_id)
            transfer_finished = True
            self.add_change(
                Change(
                    context_key,
                    'initiated',
                    context={context_key: transfer_finished},
                    rollback=Change.noop_rollback,
                )
            )

        should_sleep = False
        context_key = 'update_porto_container_secrets'
        change = None
        change_from_context = self.context_get(context_key)
        if change_from_context:
            change = DBMChange(
                change_from_context.get('operation_id'),
                change_from_context.get('deploy'),
                change_from_context.get('transfer'),
            )
        if not change:
            should_sleep = True
            secrets = {
                '/etc/yandex/mdb-deploy/deploy_version': {
                    'mode': '0644',
                    'content': '2',
                },
                '/etc/yandex/mdb-deploy/mdb_deploy_api_host': {
                    'mode': '0644',
                    'content': deploy_dataplane_host(self.config),
                },
            }
            change = self.dbm.update_container(fqdn, {'secrets': secrets})
            if change.jid is None and change.operation_id is None:
                raise RuntimeError(f'Unable to init secrets deploy. DBM change: {change}')
            self.add_change(
                Change(
                    context_key,
                    'initiated',
                    context={context_key: change.to_context()},
                    rollback=Change.noop_rollback,
                ),
            )
        if change.jid:
            self.deploy_api.wait([change.jid])
        elif change.operation_id:
            self.dbm.wait_operation(change.operation_id)
        if should_sleep:
            self.logger.info('Sleeping for 5 minutes to allow dns ttl to expire')
            time.sleep(300)

    def _recreate_leg(self):
        """
        Create leg again.
        """
        create_executer = self.get_create_executer()
        create_executer.run()

    def _resetup(self):
        self.drop_deploy_data(self.fqdn)

        if not self.preserve:
            container = self.dbm.get_container(self.fqdn)
            if container.get("dom0") in (self.resetup_from or None, None):
                self.prepare_porto_container(self.fqdn)

        self._recreate_leg()


def resetup_args_from_task_args(task_args: dict) -> ResetupArgs:
    return ResetupArgs(
        fqdn=task_args['fqdn'],
        cid=task_args['cid'],
        preserve_if_possible=task_args['preserve_if_possible'],
        ignore_hosts=task_args['ignore_hosts'],
        lock_taken=task_args[SKIP_LOCK_CREATION_ARG],
        try_save_disks=task_args['try_save_disks'],
        resetup_from=task_args['resetup_from'],
    )


def init_routine(
    config, task, queue, routine_class_map: dict, resetup_args: ResetupArgs, resetup_action: str
) -> Resetup:
    env_name = get_env_name_from_config(config)
    try:
        action_type = ResetupActionType(resetup_action)
    except ValueError:
        raise ExposedException(
            'Unknown resetup_action "{}". Only {} are available'.format(
                resetup_action, ', '.join([ResetupActionType.READD.value, ResetupActionType.RESTORE.value])
            )
        )
    task_type = task['task_type']
    try:
        routine_class = routine_class_map[(env_name, action_type, task_type)]
    except KeyError:
        raise ExposedException(
            'Could not find resetuper, probably you should write one for {env} {action} {task}'.format(
                env=env_name,
                action=action_type,
                task=task_type,
            )
        )
    logging.getLogger(__name__).info(
        'Chose %s for args: %s, %s, %s', routine_class.__name__, env_name, action_type, task_type
    )
    return routine_class(config, task, queue, resetup_args)
