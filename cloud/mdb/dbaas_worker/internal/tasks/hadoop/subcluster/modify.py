"""
Hadoop Create subcluster modify executor
"""
import time

from cloud.mdb.dbaas_worker.internal.exceptions import UserExposedException
from cloud.mdb.dbaas_worker.internal.tasks.utils import build_host_group, register_executor
from cloud.mdb.dbaas_worker.internal.tasks.hadoop.cluster.modify import HadoopClusterModify
from cloud.mdb.dbaas_worker.internal.tasks.hadoop.constants import DATAPROC_MANAGER_FLAG
from cloud.mdb.dbaas_worker.internal.tasks.hadoop.utils import (
    set_hosts_params_from_pillar,
    split_by_subcid,
    update_group_meta,
)

MODIFY = "hadoop_subcluster_modify"


class HadoopClusterModifyError(UserExposedException):
    """
    Base error
    """


@register_executor(MODIFY)
class HadoopSubclusterModify(HadoopClusterModify):
    """
    Modify nodes of dataproc subcluster
    """

    def run(self):
        hosts_to_modify = self.args['hosts']
        subcluster, other = split_by_subcid(hosts_to_modify, self.args['subcid'])
        host_group = build_host_group(self.config.hadoop, subcluster)
        other = build_host_group(self.config.hadoop, other)
        self._desired_security_groups_state([host_group, other], managed=False)

        if 'instance_group_subclusters' in self.args:
            self._modify_instance_group_subclusters()
            finished_updating_compute_resources_at = int(time.time())
            if DATAPROC_MANAGER_FLAG in self.task['feature_flags']:
                self.dataproc_manager.cluster_wait(
                    self.task['cid'], timeout=60 * 30, updated_after=finished_updating_compute_resources_at
                )
            return

        hosts_to_decommission = self.args.get('hosts_to_decommission')
        if hosts_to_decommission:
            self._delete_hosts()

        host_group.properties.compute_image_id = self.args['image_id']
        host_group.properties.security_group_ids = self.desired_security_groups
        self._update_host_group(
            host_group,
            decommission_timeout=self.args.get('decommission_timeout'),
        )

        set_hosts_params_from_pillar(
            other, self.task['folder_id'], self.internal_api, validate_service_account_via=self.resource_manager
        )
        update_group_meta(other, self.user_compute_api)

        if DATAPROC_MANAGER_FLAG in self.task['feature_flags']:
            finished_updating_compute_resources_at = int(time.time())
            self.dataproc_manager.hosts_wait(self.task['cid'], subcluster.keys(), timeout=60 * 30)
            self.dataproc_manager.cluster_wait(
                self.task['cid'], timeout=60 * 30, updated_after=finished_updating_compute_resources_at
            )
