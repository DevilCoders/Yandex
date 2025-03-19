"""
Hadoop Delete subcluster executor
"""
from cloud.mdb.dbaas_worker.internal.providers.dataproc_manager import ClusterNotHealthyError
from cloud.mdb.dbaas_worker.internal.providers.instance_group import InstanceGroupNotFoundError
from cloud.mdb.dbaas_worker.internal.tasks.utils import build_host_group, build_host_group_from_list, register_executor
from cloud.mdb.dbaas_worker.internal.tasks.hadoop.cluster.delete import HadoopClusterDelete
from cloud.mdb.dbaas_worker.internal.tasks.hadoop.constants import DATAPROC_MANAGER_FLAG
from cloud.mdb.dbaas_worker.internal.tasks.hadoop.utils import (
    delete_unmanaged_host_group,
    set_hosts_params_from_pillar,
    update_group_meta,
)


DELETE = "hadoop_subcluster_delete"


@register_executor(DELETE)
class HadoopSubclusterDelete(HadoopClusterDelete):
    """
    Delete hadoop subcluster
    """

    def run(self):
        instance_group_id = self.args.get('instance_group_id')
        if instance_group_id:
            self.instance_group.work_as_worker_service_account()
            try:
                # secwall's proposition to make 404 in monitorings during get instead of during delete
                # if a group is not found (this normally not happen)
                self.instance_group.get(instance_group_id)
                self.instance_group.delete(instance_group_id, referrer_id=self.args['subcid'])
            except InstanceGroupNotFoundError:
                self.logger.error(f'Group {instance_group_id} was not found during deletion')

        else:
            subcluster = build_host_group_from_list(self.config.hadoop, self.args['host_list'])
            decommission_timeout = int(self.args.get('decommission_timeout') or 0)
            if decommission_timeout:
                fqdns = [host_info['fqdn'] for host_info in self.args['host_list']]
                self.dataproc_manager.decommission_hosts(
                    cid=self.task['cid'],
                    hosts=fqdns,
                    timeout=decommission_timeout,
                    only_yarn=True,
                )
                try:
                    self.dataproc_manager.hosts_wait(
                        cid=self.task['cid'],
                        hosts=fqdns,
                        statuses=(self.dataproc_manager.DECOMMISSIONED, self.dataproc_manager.UNKNOWN),
                        timeout=decommission_timeout,
                        service=self.dataproc_manager.YARN,
                    )
                except ClusterNotHealthyError:
                    self.logger.exception('Decommission did not finish until timeout')

            delete_unmanaged_host_group(
                self.user_compute_api,
                self.dns_api,
                subcluster,
                self.config.hadoop.running_operations_limit,
            )

        other = build_host_group(self.config.hadoop, self.args['hosts'])
        set_hosts_params_from_pillar(other, self.task['folder_id'], self.internal_api)
        update_group_meta(other, self.user_compute_api)

        if DATAPROC_MANAGER_FLAG in self.task['feature_flags']:
            if 'host_list' in self.args:
                self.dataproc_manager.hosts_wait(
                    self.task['cid'], subcluster.hosts.keys(), statuses=self.dataproc_manager.UNKNOWN, timeout=60 * 30
                )
            self.dataproc_manager.cluster_wait(self.task['cid'], timeout=60 * 30)
