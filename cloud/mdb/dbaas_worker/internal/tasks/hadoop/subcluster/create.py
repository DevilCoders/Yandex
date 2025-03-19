"""
Hadoop Create subcluster executor
"""
import time

from cloud.mdb.dbaas_worker.internal.tasks.utils import build_host_group, register_executor
from cloud.mdb.dbaas_worker.internal.tasks.hadoop.cluster.create import HadoopClusterCreate
from cloud.mdb.dbaas_worker.internal.tasks.hadoop.constants import DATAPROC_MANAGER_FLAG
from cloud.mdb.dbaas_worker.internal.tasks.hadoop.utils import (
    get_create_vms_arguments,
    set_hosts_params_from_pillar,
    split_by_subcid,
    update_group_meta,
    get_cluster_pillar,
    COMPUTE_ROLE_TYPE,
)

CREATE = "hadoop_subcluster_create"


@register_executor(CREATE)
class HadoopSubclusterCreate(HadoopClusterCreate):
    """
    Create hadoop subcluster in compute
    """

    def run(self):
        subcluster_host_group, other = split_by_subcid(self.args['hosts'], self.args['subcid'])

        subcluster_host_group = build_host_group(self.config.hadoop, subcluster_host_group)
        subcluster_host_group.properties.compute_image_id = self.args['image_id']
        self._desired_security_groups_state([subcluster_host_group], managed=False)

        cluster_pillar = get_cluster_pillar(hosts=self.args['hosts'], internal_api=self.internal_api)
        subcluster_config = cluster_pillar['data']['topology']['subclusters'][self.args['subcid']]
        subcluster_role = subcluster_config['role']
        if subcluster_role == COMPUTE_ROLE_TYPE and 'instance_group_config' in subcluster_config:
            self._create_instance_group_subcluster(cluster_pillar)
            host_group = build_host_group(self.config.hadoop, self.args['hosts'])
            set_hosts_params_from_pillar(
                host_group,
                self.task['folder_id'],
                self.internal_api,
                validate_service_account_via=self.resource_manager,
                security_group_ids=self.desired_security_groups,
            )
            update_group_meta(host_group, self.user_compute_api)
            if DATAPROC_MANAGER_FLAG in self.task['feature_flags']:
                finished_updating_compute_resources_at = int(time.time())
                self.dataproc_manager.cluster_wait(
                    self.task['cid'], timeout=60 * 30, updated_after=finished_updating_compute_resources_at
                )
            return

        set_hosts_params_from_pillar(
            subcluster_host_group,
            self.task['folder_id'],
            self.internal_api,
            validate_service_account_via=self.resource_manager,
            security_group_ids=self.desired_security_groups,
        )

        create_vms_attributes_list = []
        for create_vm_args in get_create_vms_arguments(subcluster_host_group):
            create_vms_attributes_list.append(create_vm_args)
        self._create_unmanaged_vms(create_vms_attributes_list)
        self.gpg_backup_key.exists(getattr(subcluster_host_group.properties, 'gpg_target_id', self.task['cid']))

        other = build_host_group(self.config.hadoop, other)
        set_hosts_params_from_pillar(
            other, self.task['folder_id'], self.internal_api, validate_service_account_via=self.resource_manager
        )
        update_group_meta(other, self.user_compute_api)

        if DATAPROC_MANAGER_FLAG in self.task['feature_flags']:
            finished_updating_compute_resources_at = int(time.time())
            self.dataproc_manager.hosts_wait(self.task['cid'], subcluster_host_group.hosts.keys(), timeout=60 * 30)
            self.dataproc_manager.cluster_wait(
                self.task['cid'], timeout=60 * 30, updated_after=finished_updating_compute_resources_at
            )
