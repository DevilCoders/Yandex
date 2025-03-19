"""
Hadoop Create cluster executor
"""
from typing import Dict

from cloud.mdb.dbaas_worker.internal.exceptions import UserExposedException
from cloud.mdb.dbaas_worker.internal.providers.dataproc_manager import DataprocManager
from cloud.mdb.dbaas_worker.internal.providers.instance_group import InstanceGroup
from cloud.mdb.dbaas_worker.internal.providers.dns import Record
from cloud.mdb.dbaas_worker.internal.providers.user_compute import UserComputeApi
from cloud.mdb.dbaas_worker.internal.tasks.common.cluster.create import ClusterCreateExecutor
from cloud.mdb.dbaas_worker.internal.tasks.utils import build_host_group, register_executor
from cloud.mdb.dbaas_worker.internal.tasks.hadoop.constants import DATAPROC_MANAGER_FLAG
from cloud.mdb.dbaas_worker.internal.tasks.hadoop.utils import (
    classify_host_map,
    get_create_vms_arguments,
    set_hosts_params_from_pillar,
    COMPUTE_ROLE_TYPE,
    get_instance_group_config,
    get_cluster_pillar,
)


CREATE = "hadoop_cluster_create"


class HadoopClusterCreateError(UserExposedException):
    """
    Base service accounts error
    """


@register_executor(CREATE)
class HadoopClusterCreate(ClusterCreateExecutor):
    """
    Create hadoop cluster in compute
    """

    def __init__(self, config, task, queue, args):
        super().__init__(config, task, queue, args)
        self.dataproc_manager = DataprocManager(self.config, self.task, self.queue)
        self.user_compute_api = UserComputeApi(self.config, self.task, self.queue)
        self.instance_group = InstanceGroup(self.config, self.task, self.queue)

    def _finish_unmanaged_host_creation(self, host: str):
        self.user_compute_api.instance_wait(host)
        self.user_compute_api.save_instance_meta(host)
        self._create_unmanaged_record(host)

    def _finish_unmanaged_vm_creation(self, operation_ids_to_hosts: dict):
        completed_operation_id = self.user_compute_api.wait_for_any_finished_operation(
            operation_ids_to_hosts, timeout=1800.0
        )
        host = operation_ids_to_hosts[completed_operation_id]
        del operation_ids_to_hosts[completed_operation_id]
        self._finish_unmanaged_host_creation(host)

    def _create_unmanaged_record(self, host):
        """
        Create managed and public dns records for unnmanaged hosts
        """
        public_records = []
        for address, version in self.user_compute_api.get_instance_public_addresses(host):
            public_records.append(Record(address=address, record_type=('AAAA' if version == 6 else 'A')))
        self.dns_api.set_records(host, public_records)

    def _create_unmanaged_vms(self, create_vms_attributes_list: list):
        operation_ids_to_hosts: Dict[str, str] = {}
        for vm_create_attributes in create_vms_attributes_list:
            if len(operation_ids_to_hosts) == self.config.hadoop.running_operations_limit:
                self._finish_unmanaged_vm_creation(operation_ids_to_hosts)

            operation_id = self.user_compute_api.instance_exists(**vm_create_attributes)
            if operation_id:
                host = vm_create_attributes['fqdn']
                operation_ids_to_hosts[operation_id] = host

        while operation_ids_to_hosts:
            self._finish_unmanaged_vm_creation(operation_ids_to_hosts)

    def _create_instance_group_subclusters(self, cluster_pillar):
        for subcluster_id, subcluster_pillar in cluster_pillar['data']['topology']['subclusters'].items():
            if subcluster_pillar['role'] == COMPUTE_ROLE_TYPE and 'instance_group_config' in subcluster_pillar:
                self._create_instance_group_subcluster(cluster_pillar, subcid=subcluster_id)

    def _create_instance_group_subcluster(self, cluster_pillar, subcid=None):
        subcluster_id = self.args.get('subcid', subcid)
        cluster_pillar['data']['image'] = self.args['image_id']
        host_group_ids = list(self.args['hosts'].values())[0]['host_group_ids']
        subnet_id = cluster_pillar['data']['topology']['subclusters'][subcluster_id]['subnet_id']
        is_dualstack_subnet = self.user_compute_api._vpc_provider.get_subnet(subnet_id).v6_cidr_blocks
        instance_group_config = get_instance_group_config(
            subcid=subcluster_id,
            pillar=cluster_pillar,
            security_group_ids=self.desired_security_groups,
            host_group_ids=host_group_ids,
            is_dualstack_subnet=is_dualstack_subnet,
        )
        self.instance_group.work_as_worker_service_account()
        self.instance_group.exists(
            folder_id=self.task['folder_id'],
            instance_group_config=instance_group_config,
            subcluster_id=subcluster_id,
        )

    def run(self):
        hosts = self.args['hosts']
        master_hosts, data_hosts, compute_hosts = classify_host_map(hosts)

        master_host_group = build_host_group(self.config.hadoop, master_hosts)
        data_host_group, compute_host_group = None, None
        if data_hosts:
            data_host_group = build_host_group(self.config.hadoop, data_hosts)
        if compute_hosts:
            compute_host_group = build_host_group(self.config.hadoop, compute_hosts)
        self._desired_security_groups_state([master_host_group, data_host_group, compute_host_group], managed=False)

        create_vms_attributes_list = []

        for group in [master_host_group, data_host_group, compute_host_group]:
            if group is not None:
                group.properties.compute_image_id = self.args['image_id']
                group.properties.security_groups = self.desired_security_groups

                set_hosts_params_from_pillar(
                    group,
                    self.task['folder_id'],
                    self.internal_api,
                    validate_service_account_via=self.resource_manager,
                    security_group_ids=self.desired_security_groups,
                )
                for create_vm_args in get_create_vms_arguments(group):
                    create_vms_attributes_list.append(create_vm_args)

        self._create_unmanaged_vms(create_vms_attributes_list)

        cluster_pillar = get_cluster_pillar(hosts=self.args['hosts'], internal_api=self.internal_api)
        self._create_instance_group_subclusters(cluster_pillar)

        for host_group in [master_host_group, data_host_group, compute_host_group]:
            if host_group is not None and host_group.properties.create_gpg:
                self.gpg_backup_key.exists(getattr(host_group.properties, 'gpg_target_id', self.task['cid']))

        timeout = 60 * 30
        for init_act in cluster_pillar['data'].get('initialization_actions', []):
            timeout += init_act.get('timeout', 600)

        if DATAPROC_MANAGER_FLAG in self.task['feature_flags']:
            self.dataproc_manager.cluster_wait(self.task['cid'], timeout=timeout)
