"""
Host resetup tool
"""

import json
import logging
import string
import time
from copy import deepcopy
from datetime import datetime
from secrets import choice
from traceback import format_exc
from typing import Optional

from dbaas_common.dict import combine_dict

from cloud.mdb.dbaas_worker.internal.providers.base_metadb import BaseMetaDBProvider
from cloud.mdb.dbaas_worker.internal.providers.compute import ComputeApi
from cloud.mdb.dbaas_worker.internal.providers.conductor import ConductorApi, ConductorError
from cloud.mdb.dbaas_worker.internal.providers.dbm import DBMApi
from cloud.mdb.dbaas_worker.internal.providers.deploy import DeployAPI, deploy_dataplane_host
from cloud.mdb.dbaas_worker.internal.providers.internal_api import InternalApi
from cloud.mdb.dbaas_worker.internal.providers.juggler import JugglerApi
from cloud.mdb.dbaas_worker.internal.providers.mlock import Mlock
from cloud.mdb.dbaas_worker.internal.providers.ssh import SSHClient
from cloud.mdb.dbaas_worker.internal.query import execute
from cloud.mdb.dbaas_worker.internal.tasks import get_executor
from cloud.mdb.dbaas_worker.internal.tasks.clickhouse import resetup as clickhouse_rstp
from cloud.mdb.dbaas_worker.internal.tasks.common.resetup_common import ResetupActionType, ResetupArgs, init_routine
from cloud.mdb.dbaas_worker.internal.tasks.elasticsearch import resetup as elasticsearch_rstp
from cloud.mdb.dbaas_worker.internal.tasks.greenplum import resetup as greenplum_rstp
from cloud.mdb.dbaas_worker.internal.tasks.kafka import resetup as kafka_rstp
from cloud.mdb.dbaas_worker.internal.tasks.mongodb import resetup as mongodb_rstp
from cloud.mdb.dbaas_worker.internal.tasks.mysql import resetup as mysql_rstp
from cloud.mdb.dbaas_worker.internal.tasks.postgresql import resetup as postgres_rstp
from cloud.mdb.dbaas_worker.internal.tasks.redis import resetup as redis_rstp
from cloud.mdb.internal.python.compute.instances import InstanceModel
from cloud.mdb.internal.python.logs import MdbLoggerAdapter

ADD_TASK_ROLE_MAP = {
    'zk': 'clickhouse_zookeeper_host_create',
    'mongodb_cluster.mongocfg': 'mongodb_host_create',
    'mysql_cluster': 'mysql_host_create',
    'redis_cluster': 'redis_host_create',
    'clickhouse_cluster': 'clickhouse_host_create',
    'mongodb_cluster.mongos': 'mongodb_host_create',
    'postgresql_cluster': 'postgresql_host_create',
    'mongodb_cluster.mongod': 'mongodb_host_create',
    'mongodb_cluster.mongoinfra': 'mongodb_host_create',
    'kafka_cluster': 'kafka_host_create',
    'elasticsearch_cluster.datanode': 'elasticsearch_host_create',
    'elasticsearch_cluster.masternode': 'elasticsearch_host_create',
}

RESTORE_TASK_MAP = {
    'postgresql_cluster': 'postgresql_cluster_restore',
    'clickhouse_cluster': 'clickhouse_cluster_restore',
    'mongodb_cluster': 'mongodb_cluster_restore',
    'mysql_cluster': 'mysql_cluster_restore',
    'redis_cluster': 'redis_cluster_restore',
}

STOP_TASK_MAP = {
    'postgresql_cluster': 'postgresql_cluster_stop',
    'clickhouse_cluster': 'clickhouse_cluster_stop',
    'mongodb_cluster': 'mongodb_cluster_stop',
    'mysql_cluster': 'mysql_cluster_stop',
    'redis_cluster': 'redis_cluster_stop',
    'hadoop_cluster': 'hadoop_cluster_stop',
    'kafka_cluster': 'kafka_cluster_stop',
    'elasticsearch_cluster': 'elasticsearch_cluster_stop',
}

START_TASK_MAP = {
    'postgresql_cluster': 'postgresql_cluster_start',
    'clickhouse_cluster': 'clickhouse_cluster_start',
    'mongodb_cluster': 'mongodb_cluster_start',
    'mysql_cluster': 'mysql_cluster_start',
    'redis_cluster': 'redis_cluster_start',
    'hadoop_cluster': 'hadoop_cluster_start',
    'kafka_cluster': 'kafka_cluster_start',
    'elasticsearch_cluster': 'elasticsearch_cluster_start',
}


RESTORE_HANDLE_MAP = {
    'clickhouse_cluster': 'clickhouse',
    'mongodb_cluster': 'mongodb',
    'redis_cluster': 'redis',
}


class HostResetuper:
    """
    Provider for host resetup in compute
    """

    def __init__(self, config, task, queue):
        task = {
            'task_id': 'host-resetup',
            'timeout': 7 * 24 * 3600,
            'changes': [],
            'context': {},
            'feature_flags': [],
            'folder_id': 'test',
        }
        self.task = task
        self.conductor = ConductorApi(config, task, queue)
        self.metadb = BaseMetaDBProvider(config, task, queue)
        self.deploy_api = DeployAPI(config, task, queue)
        self.juggler_api = JugglerApi(config, task, queue)
        self.mlock = Mlock(config, task, queue)
        self.compute = ComputeApi(config, task, queue)
        self.dbm = DBMApi(config, task, queue)
        self.ssh = SSHClient(config, task, queue)
        self.config = config
        self.queue = queue
        self.logger = MdbLoggerAdapter(logging.getLogger(__name__), extra={})

    def stop_instance(self, instance: InstanceModel, context_suffix='stop'):
        """
        Stop instance
        """
        operation = self.compute.instance_stopped(instance.fqdn, instance.id, context_suffix=context_suffix)
        if operation:
            self.compute.operation_wait(operation)

    def save_disks(self, fqdn):
        """
        Mark instance disks undeletable
        """
        try:
            instance = self.compute.get_instance(fqdn)
            if not instance:
                raise RuntimeError(f'Unable to find instance {fqdn}')

            self.stop_instance(instance)

            operation = self.compute.set_autodelete_for_boot_disk(instance.id, instance.boot_disk.disk_id, False)
            self.logger.info('Saved boot disk: %s', instance.boot_disk.disk_id)
            if operation:
                self.compute.operation_wait(operation)

            for disk_id in {x.disk_id for x in instance.secondary_disks}:
                operation = self.compute.detach_disk(instance.id, disk_id)
                self.logger.info('Saved secondary disk: %s', disk_id)
                if operation:
                    self.compute.operation_wait(operation)
        except Exception as exc:
            self.logger.error(f'Unable to save instance disks: {exc!r}')

    def drop_deploy_data(self, fqdn):
        """
        Drop deploy v1/v2 data for minion
        """
        if self.deploy_api.has_minion(fqdn):
            self.deploy_api.unregister_minion(fqdn)
        else:
            self.deploy_api.create_minion(fqdn, self.config.deploy.group)

    def drop_instance(self, fqdn):
        """
        Delete instance
        """
        self.compute.operation_wait(self.compute.instance_absent(fqdn))

    def get_base_task_args(self, cluster_id, ignore):
        """
        Get common task args (e.g. hosts)
        """
        args = {'hosts': {}, 'cid': cluster_id}
        with self.metadb.get_master_conn() as conn:
            with conn:
                cur = conn.cursor()
                res = execute(cur, 'generic_resolve', cid=cluster_id)
        for row in res:
            if ignore and row['fqdn'] in ignore:
                continue
            args['hosts'][row['fqdn']] = {x: row[x] for x in row if x not in ['name', 'fqdn']}

        return args

    def get_host_info(self, fqdn):
        """
        Get host info (cluster_id, cluster_type, instance_id)
        """
        with self.metadb.get_master_conn() as conn:
            with conn:
                cur = conn.cursor()
                res = execute(cur, 'get_host_info', fqdn=fqdn)
                return res[0]

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

    def get_mysql_zk_hosts(self, cluster_id):
        """
        Get zk_hosts from cluster pillar for mysql
        """
        with self.metadb.get_master_conn() as conn:
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

    def get_zid(self, subcid, fqdn):
        """
        Get zookeeper id for host in cluster
        """
        with self.metadb.get_master_conn() as conn:
            with conn:
                cur = conn.cursor()
                res = execute(
                    cur, 'get_pillar', path=['data', 'zk', 'nodes'], cid=None, subcid=subcid, fqdn=None, shard_id=None
                )
                return res[0]['value'][fqdn]

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

    def get_restore_args(self, admin_api_url, fqdn, ignore):
        """
        Get restore task args
        """
        info = self.get_host_info(fqdn)
        args = self.get_base_task_args(info['cid'], ignore)

        shard_id = args['hosts'][fqdn]['shard_id']

        if info['type'] == 'clickhouse_cluster':
            if len([x for x in args['hosts'] if args['hosts'][x]['shard_id'] == shard_id]) != 1:
                raise RuntimeError('Restoring shard with more than one host will fail')
            for host, opts in args['hosts'].copy().items():
                if opts.get('shard_id') is not None and opts.get('shard_id') != shard_id:
                    del args['hosts'][host]
        elif len(args['hosts']) != 1:
            raise RuntimeError('Restoring cluster with more than one host will fail')

        task_type = RESTORE_TASK_MAP[info['type']]
        args.update(
            {
                's3_bucket': self.get_s3_bucket(info['cid']),
                'target-pillar-id': self.get_target_pillar_id(info['cid'], args['hosts'][fqdn]['subcid']),
            }
        )

        if info['type'] == 'mysql_cluster':
            args.update(
                {
                    'zk_hosts': self.get_mysql_zk_hosts(info['cid']),
                    'restore-from': {
                        'cid': info['cid'],
                        'time': f'{datetime.utcnow().isoformat()}+00:00',
                        'backup-id': 'LATEST',
                    },
                }
            )
            return info['type'], task_type, args

        if info['type'] == 'postgresql_cluster':
            args.update(
                {
                    'restore-from': {
                        'cid': info['cid'],
                        'time': f'{datetime.utcnow().isoformat()}+00:00',
                        'backup-id': 'LATEST',
                        'time-inclusive': True,
                        'restore-latest': True,
                        'rename-database-from-to': {},
                    },
                }
            )
            return info['type'], task_type, args

        admin_api = InternalApi(self.config, self.task, self.queue, api_url=admin_api_url)
        admin_api.default_headers = {'X-YaCloud-SubjectToken': self.compute.get_token()}
        backups = admin_api.get_backups(RESTORE_HANDLE_MAP[info['type']], info['cid'])

        shard_name = args['hosts'][fqdn]['shard_name']
        found = False
        for backup in backups['backups']:
            if shard_id is None:
                backup_info = backup
                found = True
                break
            if shard_name in backup['sourceShardNames']:
                backup_info = backup
                found = True
                break

        if not found:
            raise RuntimeError(f'No backup to restore host {fqdn}')

        backup_id = backup_info['id'].replace(f'{info["cid"]}:', '').replace(f'{shard_id}:', '')

        if info['type'] == 'clickhouse_cluster':
            args.update(
                {
                    'restore-from': {
                        'cid': info['cid'],
                        's3-path': f'ch_backup/{info["cid"]}/{shard_name}/{backup_id}',
                        'backup-id': backup_id,
                    },
                }
            )
        elif info['type'] == 'mongodb_cluster':
            # TODO: pitr support
            args.update(
                {
                    'restore-from': {
                        'cid': info['cid'],
                        's3-path': f'mongodb-backup/{info["cid"]}/{shard_id}',
                        'backup-id': backup_id,
                    },
                }
            )
        elif info['type'] == 'redis_cluster':
            args.update(
                {
                    'restore-from': {
                        'cid': info['cid'],
                        'backup-id': backup_id,
                    },
                }
            )
        else:
            raise RuntimeError(f'Unable to resolve args for {info["type"]}')

        return info['type'], task_type, args

    def get_add_args(self, _, fqdn, ignore):
        """
        Get host add task and task args
        """
        info = self.get_host_info(fqdn)
        args = self.get_base_task_args(info['cid'], ignore)

        subcid = args['hosts'][fqdn]['subcid']
        shard_id = args['hosts'][fqdn]['shard_id']

        if info['type'] == 'clickhouse_cluster':
            args['resetup_from_replica'] = True
            if len([x for x in args['hosts'] if args['hosts'][x]['shard_id'] == shard_id]) == 1:
                raise RuntimeError('Adding hosts into shard with one host will fail')
            zk_hosts = [x for x in args['hosts'] if 'zk' in args['hosts'][x]['roles']]
            if zk_hosts:
                args['zk_hosts'] = ','.join([f'{host}:2181' for host in zk_hosts])
                if 'zk' in args['hosts'][fqdn]['roles']:
                    args['zid_added'] = self.get_zid(subcid, fqdn)
        elif len(args['hosts']) == 1:
            raise RuntimeError('Adding hosts into cluster with one host will fail')

        roles = args['hosts'][fqdn]['roles']
        task_type = ADD_TASK_ROLE_MAP[roles[0]]
        args['host'] = fqdn
        args['subcid'] = subcid
        args['shard_id'] = shard_id

        if info['type'] == 'mysql_cluster':
            args['zk_hosts'] = self.get_mysql_zk_hosts(info['cid'])

        return info['type'], task_type, args

    def run_task(self, task_type, args):
        """
        Run task with fake executor
        """
        task = deepcopy(self.task)
        task['task_type'] = task_type
        task['cid'] = args['cid']

        executor = get_executor(task, self.config, self.queue, args)
        try:
            executor.run()
        except Exception as exc:
            executor.rollback(exc, format_exc(), initially_failed=True)
            raise

    def get_porto_containers(self, dom0):
        """
        Get a list of porto containers on dom0
        """
        res = self.dbm.find_containers(query={'dom0': dom0})

        return [x['fqdn'] for x in res]

    def prepare_porto_container(self, fqdn):
        """
        Recreate porto container on new dom0
        """
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

        change = self.dbm.update_container(fqdn, data=dict(dom0=None))

        if change.transfer is None:
            raise RuntimeError(f'Unable to init transfer. DBM change: {change}')

        self.dbm.finish_transfer(change.transfer['id'])

        change = self.dbm.update_container(fqdn, {'secrets': secrets})

        if change.jid is None and change.operation_id is None:
            raise RuntimeError(f'Unable to init secrets deploy. DBM change: {change}')

        if change.jid:
            self.deploy_api.wait([change.jid])
        elif change.operation_id:
            self.dbm.wait_operation(change.operation_id)

        self.logger.info('Sleeping for 5 minutes to allow dns ttl to expire')
        time.sleep(300)

    def prepare_offline_cluster(self, cluster_type, args, fqdn):
        """
        Start non-target instances with disabled billing
        """
        stop_task_type = STOP_TASK_MAP[cluster_type]
        start_task_type = START_TASK_MAP[cluster_type]
        disable_operations = []
        hosts = [x for x in args['hosts'] if x != fqdn]
        for host in hosts:
            operation = self.compute.disable_billing(host)
            disable_operations.append(operation)

        for i in disable_operations:
            self.compute.operation_wait(i)

        start_args = deepcopy(args)
        del start_args['hosts'][fqdn]
        if start_args['hosts']:
            self.run_task(start_task_type, start_args)

        args['create_offline'] = True
        return stop_task_type

    def shutdown_offline_cluster(self, task_type, args):
        """
        Run stop cluster operation and enable billing on instances
        """
        self.run_task(task_type, args)

        enable_operations = []
        for host in args['hosts']:
            operation_id = self.compute.enable_billing(host)
            if operation_id:
                enable_operations.append(operation_id)

        for i in enable_operations:
            self.compute.operation_wait(i)

    def resetup(self, get_args_fun, admin_api_url, fqdn, ignore, save, offline, preserve):
        """
        Run resetup
        """
        cluster_type, task_type, args = get_args_fun(admin_api_url, fqdn, ignore)
        vtype = args['hosts'][fqdn]['vtype']

        if vtype == 'compute':
            if fqdn.endswith(self.config.postgresql.managed_zone):
                raise RuntimeError(f'Managed hostname {fqdn} used')
            if save:
                self.save_disks(fqdn)

        if offline and vtype == 'compute':
            stop_task_type = self.prepare_offline_cluster(cluster_type, args, fqdn)

        self.drop_deploy_data(fqdn)

        if vtype == 'compute':
            self.drop_instance(fqdn)
        elif vtype == 'porto' and not preserve:
            self.prepare_porto_container(fqdn)

        self.run_task(task_type, args)

        if offline and vtype == 'compute':
            self.shutdown_offline_cluster(stop_task_type, args)

    def restore_from_backup(self, fqdn, ignore=None, save=False, offline=False, preserve=False):
        """
        Restore host from latest backup
        """
        info = self.get_host_info(fqdn)
        try:
            self.run_supported(
                action=ResetupActionType.RESTORE,
                info=info,
                fqdn=fqdn,
                ignore=ignore,
                save=save,
                offline=offline,
                preserve=preserve,
            )
            return
        except ValueError:
            if ignore is None:
                ignore = set()
            self.resetup(self.get_restore_args, self.get_admin_api_host(), fqdn, ignore, save, offline, preserve)

    def get_admin_api_host(self):
        group_name = self.config.main.admin_api_conductor_group
        try:
            hosts = self.conductor.group_to_hosts(group_name)
        except ConductorError as exc:
            raise Exception('admin api host is not available in conductor: {}'.format(str(exc)))
        if len(hosts) == 0:
            raise Exception('add some admin api hosts to "{}"?'.format(group_name))
        return 'https://' + hosts[0]

    def run_supported(
        self, action: ResetupActionType, info: dict, fqdn: str, ignore=None, save=False, offline=False, preserve=False
    ):
        SUPPORTED_CLUSTERS = {
            'postgresql_cluster',
            'mysql_cluster',
            'clickhouse_cluster',
            'mongodb_cluster',
            'redis_cluster',
            'kafka_cluster',
            'elasticsearch_cluster',
            'greenplum_cluster',
        }
        if info['type'] not in SUPPORTED_CLUSTERS:
            raise ValueError
        routine_map: Optional[dict]
        if info['type'] == 'postgresql_cluster':
            routine_map = postgres_rstp.ROUTINE_MAP
            if offline:
                self.task['task_type'] = postgres_rstp.TaskType.postgres_offline
            else:
                self.task['task_type'] = postgres_rstp.TaskType.postgres_online
        elif info['type'] == 'greenplum_cluster':
            routine_map = greenplum_rstp.ROUTINE_MAP
            if offline:
                self.task['task_type'] = greenplum_rstp.TaskType.greenplum_offline
            else:
                self.task['task_type'] = greenplum_rstp.TaskType.greenplum_online
        elif info['type'] == 'mysql_cluster':
            routine_map = mysql_rstp.ROUTINE_MAP
            if offline:
                self.task['task_type'] = mysql_rstp.TaskType.mysql_offline
            else:
                self.task['task_type'] = mysql_rstp.TaskType.mysql_online
        elif info['type'] == 'clickhouse_cluster':
            routine_map = clickhouse_rstp.ROUTINE_MAP
            if offline:
                self.task['task_type'] = clickhouse_rstp.TaskType.clickhouse_offline
            else:
                self.task['task_type'] = clickhouse_rstp.TaskType.clickhouse_online
        elif info['type'] == 'kafka_cluster':
            routine_map = kafka_rstp.ROUTINE_MAP
            if offline:
                self.task['task_type'] = kafka_rstp.TaskType.kafka_offline
            else:
                self.task['task_type'] = kafka_rstp.TaskType.kafka_online
        elif info['type'] == 'redis_cluster':
            routine_map = redis_rstp.ROUTINE_MAP
            if offline:
                self.task['task_type'] = redis_rstp.TaskType.redis_offline
            else:
                self.task['task_type'] = redis_rstp.TaskType.redis_online
        elif info['type'] == 'elasticsearch_cluster':
            routine_map = elasticsearch_rstp.ROUTINE_MAP
            if offline:
                self.task['task_type'] = elasticsearch_rstp.TaskType.elastic_offline
            else:
                self.task['task_type'] = elasticsearch_rstp.TaskType.elastic_online
        elif info['type'] == 'mongodb_cluster':
            routine_map = mongodb_rstp.ROUTINE_MAP
            if offline:
                self.task['task_type'] = mongodb_rstp.TaskType.mongo_offline
            else:
                self.task['task_type'] = mongodb_rstp.TaskType.mongo_online
        else:
            raise ValueError
        routine = init_routine(
            config=self.config,
            task=self.task,
            queue=self.queue,
            routine_class_map=routine_map,
            # Yep. It's really broken:
            #
            #   error: Missing positional argument "resetup_from" in call to "ResetupArgs"
            #
            # but I don't know what parts from that class can be used
            # At least move_container (and probably free_dom0) still works.
            resetup_args=ResetupArgs(  # type:ignore
                fqdn=fqdn,
                cid=info['cid'],
                preserve_if_possible=preserve,
                ignore_hosts=ignore or set(),
                lock_taken=True,
                try_save_disks=save,
                resetup_from='',
            ),
            resetup_action=action.value,
        )
        routine.resetup_with_lock()
        return

    def readd(self, fqdn, ignore=None, save=False, offline=False, preserve=False):
        """
        Readd host into cluster
        """
        info = self.get_host_info(fqdn)
        try:
            self.run_supported(
                action=ResetupActionType.READD,
                info=info,
                fqdn=fqdn,
                ignore=ignore,
                save=save,
                offline=offline,
                preserve=preserve,
            )
            return
        except ValueError:
            if ignore is None:
                ignore = set()
            self.resetup(self.get_add_args, None, fqdn, ignore, save, offline, preserve)
