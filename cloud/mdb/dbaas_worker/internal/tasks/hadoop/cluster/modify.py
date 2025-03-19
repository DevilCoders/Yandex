"""
Hadoop Modify cluster executor
"""
import time

from cloud.mdb.dbaas_worker.internal.exceptions import UserExposedException
from cloud.mdb.dbaas_worker.internal.providers.compute import get_core_fraction
from cloud.mdb.dbaas_worker.internal.providers.dataproc_manager import ClusterNotHealthyError, DataprocManager
from cloud.mdb.dbaas_worker.internal.providers.instance_group import InstanceGroup
from cloud.mdb.dbaas_worker.internal.providers.internal_api import InternalApi
from cloud.mdb.dbaas_worker.internal.providers.resource_manager import ResourceManagerApi
from cloud.mdb.dbaas_worker.internal.providers.user_compute import UserComputeApi
from cloud.mdb.dbaas_worker.internal.tasks.common.cluster.modify import ClusterModifyExecutor
from cloud.mdb.dbaas_worker.internal.tasks.utils import (
    build_host_group,
    build_host_group_from_list,
    register_executor,
)
from cloud.mdb.dbaas_worker.internal.tasks.hadoop.constants import DATAPROC_MANAGER_FLAG
from cloud.mdb.dbaas_worker.internal.tasks.hadoop.utils import (
    delete_unmanaged_host_group,
    get_cluster_pillar,
    get_instance_group_config,
    set_hosts_params_from_pillar,
    split_by_subcids,
    render_user_data,
)
from cloud.mdb.internal.python.compute.disks import DiskType

MODIFY = "hadoop_cluster_modify"


class HadoopClusterModifyError(UserExposedException):
    """
    Base service accounts error
    """


@register_executor(MODIFY)
class HadoopClusterModify(ClusterModifyExecutor):
    """
    Modify hadoop cluster in compute
    """

    def __init__(self, config, task, queue, args):
        super().__init__(config, task, queue, args)
        self.user_compute_api = UserComputeApi(self.config, self.task, self.queue)
        self.internal_api = InternalApi(self.config, self.task, self.queue)
        self.resource_manager = ResourceManagerApi(self.config, self.task, self.queue)
        self.dataproc_manager = DataprocManager(self.config, self.task, self.queue)
        self.instance_group = InstanceGroup(self.config, self.task, self.queue)

    def _stop_and_update_instance(self, host, opts, changes):
        self.user_compute_api.operation_wait(
            self.user_compute_api.instance_stopped(
                fqdn=host,
                instance_id=opts['vtype_id'],
                referrer_id=opts['subcid'],
            )
        )
        modify_instance_operations = []
        if 'resources' in changes:
            modify_instance_operations.append(
                self.user_compute_api.set_instance_resources(
                    instance_id=opts['vtype_id'],
                    platform_id=opts['platform_id'].replace('mdb-', 'standard-'),
                    cores=opts['cpu_limit'],
                    core_fraction=get_core_fraction(opts['cpu_guarantee'], opts['cpu_limit']),
                    gpus=opts['gpu_limit'],
                    io_cores=opts['io_cores_limit'],
                    memory=opts['memory_limit'],
                    referrer_id=opts['subcid'],
                )
            )
        if 'nbs_size_up' in changes:
            disk = self.user_compute_api.get_instance_disks(host, opts['vtype_id'])['boot']
            modify_instance_operations.append(self.user_compute_api.set_disk_size(disk.id, opts['space_limit']))
        for operation_id in modify_instance_operations:
            self.user_compute_api.operation_wait(operation_id)

    def _update_host_with_decommission(self, host, opts, changes, hosts_to_stop, decommission_timeout):
        self._stop_and_update_instance(host, opts, changes)

        # Is is needed to trigger yarn nodes refresh each time after decommissioned node is down
        # otherwise the node will start with disabled hadoop-yarn-nodemanager service.
        # When restarting last node we intentionally send decommission request
        # with empty host and non-empty timeout -- this will also trigger node refresh
        self.dataproc_manager.decommission_hosts(
            cid=self.task['cid'],
            hosts=hosts_to_stop,
            timeout=decommission_timeout,
            only_yarn=True,
        )

        # wait for yarn node refresh to ensure node is removed from decommission list before node starts
        self.dataproc_manager.wait_for_yarn_host_before_start(self.task['cid'], host)
        self.user_compute_api.operation_wait(
            self.user_compute_api.instance_running(fqdn=host, instance_id=opts['vtype_id'], referrer_id=opts['subcid'])
        )
        self.dataproc_manager.hosts_wait(self.task['cid'], [host], timeout=60 * 30)

    def _update_with_decommission(self, cluster_id, hosts_to_update_with_downtime, decommission_timeout):
        hosts_to_stop = set(hosts_to_update_with_downtime.keys())

        self.dataproc_manager.decommission_hosts(
            cid=cluster_id,
            hosts=hosts_to_stop,
            timeout=decommission_timeout,
            only_yarn=True,
        )

        while hosts_to_stop:
            try:
                host = self.dataproc_manager.get_first_host_with_status(
                    cid=cluster_id,
                    hosts=hosts_to_stop,
                    statuses=self.dataproc_manager.DECOMMISSIONED,
                    timeout=decommission_timeout,
                    service=self.dataproc_manager.YARN,
                )
                # host can be None after task interruption (context usage)
                if host:
                    opts, changes = hosts_to_update_with_downtime[host]
                    self._update_host_with_decommission(host, opts, changes, hosts_to_stop, decommission_timeout)
                    hosts_to_stop.remove(host)
                else:
                    for host, opts_and_changes in hosts_to_update_with_downtime.items():
                        opts, changes = opts_and_changes
                        self._update_host_with_decommission(host, opts, changes, hosts_to_stop, decommission_timeout)
                    break

            except ClusterNotHealthyError:
                self.logger.exception('Decommission did not finish until timeout')

    def _update_host_group(self, host_group, decommission_timeout):
        cluster_id = self.task['cid']
        create_instance_operations = {}
        attributes_update_operation_ids = []
        set_hosts_params_from_pillar(
            host_group, self.task['folder_id'], self.internal_api, validate_service_account_via=self.resource_manager
        )

        hosts_to_update_with_downtime = dict()
        security_group_ids = None
        if hasattr(host_group.properties, 'security_group_ids'):
            security_group_ids = host_group.properties.security_group_ids
        for host, opts in host_group.hosts.items():
            placement_info = self.disk_placement_groups.get_placement_info(
                DiskType(opts['disk_type_id']),
                folder_id=self.config.compute.folder_id,
                geo=opts['geo'],
                cid=self.task['cid'],
                subcid=opts['subcid'],
                shard_id=opts['shard_id'],
            )
            create_instances_operation_id = self.user_compute_api.instance_exists(
                geo=opts['geo'],
                fqdn=host,
                managed_fqdn='',
                metadata=opts['meta'],
                image_type=None,
                image_id=host_group.properties.compute_image_id,
                platform_id=opts['platform_id'].replace('mdb-', 'standard-'),
                cores=opts['cpu_limit'],
                core_fraction=get_core_fraction(opts['cpu_guarantee'], opts['cpu_limit']),
                memory=opts['memory_limit'],
                gpus=opts.get('gpu_limit', 0),
                io_cores=opts.get('io_cores_limit', 0),
                subnet_id=opts['subnet_id'],
                assign_public_ip=opts.get('assign_public_ip', False),
                service_account_id=opts['service_account_id'],
                labels=opts.get('labels'),
                root_disk_type_id=opts['disk_type_id'],
                root_disk_size=opts['space_limit'],
                security_groups=security_group_ids,
                disk_type_id='',
                disk_size=None,
                folder_id=None,
                revertable=False,
                host_group_ids=opts['host_group_ids'],
                disk_placement_group_id=placement_info.disk_placement_group_id,
            )
            if create_instances_operation_id:
                create_instance_operations[host] = create_instances_operation_id
            else:
                if not opts['vtype_id']:
                    opts['vtype_id'] = self.user_compute_api.save_instance_meta(host)
                changes = self.user_compute_api.get_instance_changes(
                    fqdn=host,
                    instance_id=opts['vtype_id'],
                    platform_id=opts['platform_id'].replace('mdb-', 'standard-'),
                    cores=opts['cpu_limit'],
                    core_fraction=get_core_fraction(opts['cpu_guarantee'], opts['cpu_limit']),
                    gpus=opts['gpu_limit'],
                    io_cores=opts['io_cores_limit'],
                    memory=opts['memory_limit'],
                    disk_type_id=None,
                    disk_size=None,
                    root_disk_size=opts['space_limit'],
                    labels=opts.get('labels'),
                    metadata=opts['meta'],
                    service_account_id=opts['service_account_id'],
                    security_group_ids=security_group_ids,
                )

                if (changes and set(changes) - set({'instance_attribute', 'instance_network'})) or self.args.get(
                    'restart', False
                ):
                    hosts_to_update_with_downtime[host] = (opts, changes)

                if 'instance_network' in changes:
                    operation_id = self.user_compute_api.ensure_instance_sg_same(host, security_group_ids)
                    if operation_id:
                        attributes_update_operation_ids.append(operation_id)

                if 'instance_attribute' in changes:
                    operation_id = self.user_compute_api.update_instance_attributes(
                        host,
                        labels=opts['labels'],
                        metadata=opts['meta'],
                        service_account_id=opts['service_account_id'],
                        referrer_id=opts['subcid'],
                        instance_id=opts['vtype_id'],
                    )
                    attributes_update_operation_ids.append(operation_id)

        if decommission_timeout:
            self._update_with_decommission(cluster_id, hosts_to_update_with_downtime, decommission_timeout)
        else:
            for host, opts_and_changes in hosts_to_update_with_downtime.items():
                opts, changes = opts_and_changes
                self._stop_and_update_instance(host, opts, changes)
                self.user_compute_api.operation_wait(
                    self.user_compute_api.instance_running(
                        fqdn=host,
                        instance_id=opts['vtype_id'],
                        referrer_id=opts['subcid'],
                    )
                )

        self.user_compute_api.operations_wait(attributes_update_operation_ids)

        for host, operation_id in create_instance_operations.items():
            self.user_compute_api.operation_wait(operation_id)
            self.user_compute_api.save_instance_meta(host)

        # By this moment we expect that all instances within host group are running.
        # This invariant may be violated in some cases: see https://st.yandex-team.ru/MDB-12272
        # So let's ensure that all instances are running
        for host, opts in host_group.hosts.items():
            self.user_compute_api.operation_wait(
                self.user_compute_api.instance_running(
                    fqdn=host,
                    instance_id=opts['vtype_id'],
                    referrer_id=opts['subcid'],
                )
            )

    def _decommission_hosts(self, fqdns, decommission_timeout):
        self.dataproc_manager.decommission_hosts(cid=self.task['cid'], hosts=fqdns, timeout=decommission_timeout)
        try:
            self.dataproc_manager.hosts_wait(
                cid=self.task['cid'],
                hosts=fqdns,
                statuses=self.dataproc_manager.DECOMMISSIONED,
                timeout=decommission_timeout,
            )
        except ClusterNotHealthyError:
            self.logger.exception('Decommission did not finish until timeout')

    def _delete_hosts(self):
        hosts_to_delete = self.args['hosts_to_decommission']
        host_group_to_decommission = build_host_group_from_list(
            self.config.hadoop,
            hosts_to_delete,
        )
        decommission_timeout = int(self.args.get('decommission_timeout') or 0)
        if decommission_timeout:
            fqdns = [host_info['fqdn'] for host_info in hosts_to_delete]
            self._decommission_hosts(fqdns, decommission_timeout)

        delete_unmanaged_host_group(
            self.user_compute_api,
            self.dns_api,
            host_group_to_decommission,
            self.config.hadoop.running_operations_limit,
        )

    def _modify_instance_group_subclusters(self):
        cluster_pillar = get_cluster_pillar(hosts=self.args['hosts'], internal_api=self.internal_api)
        for instance_group_tuple in self.args['instance_group_subclusters']:
            subcid, instance_group_id, is_downtime_needed = instance_group_tuple
            host_group_ids = list(self.args['hosts'].values())[0]['host_group_ids']
            subnet_id = cluster_pillar['data']['topology']['subclusters'][subcid]['subnet_id']
            is_dualstack_subnet = self.user_compute_api._vpc_provider.get_subnet(subnet_id).v6_cidr_blocks
            instance_group_config = get_instance_group_config(
                subcid=subcid,
                pillar=cluster_pillar,
                with_reference=False,
                security_group_ids=self.desired_security_groups,
                host_group_ids=host_group_ids,
                is_dualstack_subnet=is_dualstack_subnet,
            )
            instance_group_config_yaml = render_user_data(instance_group_config)
            self.instance_group.work_as_worker_service_account()
            operation_id = self.instance_group.update(
                instance_group_id=instance_group_id,
                yaml_config=instance_group_config_yaml,
                wait=False,
                referrer_id=subcid,
            )
            if is_downtime_needed:
                # We need to explicitly remove instances
                # because instance group is in opportunistic mode (IG service does not stop/delete instances by itself)
                # FIXME: possible race here -- need to find out how to understand when instances get in RUNNING_OUTDATED
                instances = {}
                for instance in self.instance_group.list_instances(instance_group_id=instance_group_id).data.instances:
                    if instance.status == self.instance_group.managed_instance_type.RUNNING_OUTDATED:
                        # If an instance must be recreated to make the group consistent with its configuration
                        instances[instance.fqdn] = instance.id
                if instances:
                    decommission_timeout = int(self.args.get('decommission_timeout') or 0)
                    if decommission_timeout:
                        self._decommission_hosts(instances, decommission_timeout)
                    self.instance_group.stop_instances(
                        instance_group_id=instance_group_id,
                        managed_instance_ids=list(instances.values()),
                        referrer_id=subcid,
                    )
            self.instance_group.wait(operation_id)

    def run(self):
        if self.args.get('hosts_to_decommission'):
            self._delete_hosts()

        hosts_by_subclusters = split_by_subcids(self.args['hosts'])

        host_groups = [build_host_group(self.config.hadoop, hosts) for hosts in hosts_by_subclusters.values()]
        self._desired_security_groups_state(host_groups, managed=False)

        if 'instance_group_subclusters' in self.args:
            self._modify_instance_group_subclusters()

        for host_group in host_groups:
            host_group.properties.compute_image_id = self.args['image_id']
            host_group.properties.security_group_ids = self.desired_security_groups
            self._update_host_group(
                host_group,
                decommission_timeout=self.args.get('decommission_timeout'),
            )

        if DATAPROC_MANAGER_FLAG in self.task['feature_flags']:
            finished_updating_compute_resources_at = int(time.time())
            self.dataproc_manager.hosts_wait(self.task['cid'], self.args['hosts'].keys(), timeout=60 * 30)
            self.dataproc_manager.cluster_wait(
                self.task['cid'], timeout=60 * 30, updated_after=finished_updating_compute_resources_at
            )
