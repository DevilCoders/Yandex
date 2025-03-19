"""
Hadoop Delete cluster executor
"""
from cloud.mdb.dbaas_worker.internal.providers.dataproc_manager import ClusterNotHealthyError, DataprocManager
from cloud.mdb.dbaas_worker.internal.providers.user_compute import UserComputeApi
from cloud.mdb.dbaas_worker.internal.providers.instance_group import InstanceGroup, InstanceGroupNotFoundError
from cloud.mdb.dbaas_worker.internal.tasks.common.cluster.delete import ClusterDeleteExecutor
from cloud.mdb.dbaas_worker.internal.tasks.utils import build_host_group, register_executor
from cloud.mdb.dbaas_worker.internal.tasks.hadoop.utils import MASTER_ROLE_TYPE, delete_unmanaged_host_group


@register_executor('hadoop_cluster_delete')
class HadoopClusterDelete(ClusterDeleteExecutor):
    """
    Delete hadoop cluster
    """

    def __init__(self, config, task, queue, args):
        super().__init__(config, task, queue, args)
        self.properties = self.config.hadoop
        self.dataproc_manager = DataprocManager(self.config, self.task, self.queue)
        self.user_compute_api = UserComputeApi(self.config, self.task, self.queue)
        self.instance_group = InstanceGroup(self.config, self.task, self.queue)

    def run(self):
        """
        Delete hosts from dbm/compute, conductor and solomon
        """
        hosts = self.args['hosts']
        host_group = build_host_group(self.properties, hosts)
        decommission_timeout = int(self.args.get('decommission_timeout') or 0)
        if decommission_timeout:
            fqdns = []
            for fqdn, meta in self.args['hosts'].items():
                if MASTER_ROLE_TYPE not in meta['roles']:
                    fqdns.append(fqdn)
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
            host_group,
            self.config.hadoop.running_operations_limit,
        )
        if 'instance_group_ids' in self.args:
            for instance_group_id in self.args['instance_group_ids']:
                if instance_group_id:
                    self.instance_group.work_as_worker_service_account()
                    try:
                        # secwall's proposition to make 404 in monitorings during get instead of during delete
                        # if a group is not found (this normally not happen)
                        self.instance_group.get(instance_group_id)
                        subcid = self.instance_group.metadb_instance_group.get_subcid_by_instance_group_id(
                            instance_group_id=instance_group_id,
                        )
                        self.instance_group.delete(
                            instance_group_id=instance_group_id,
                            referrer_id=subcid,
                            service_account_id=self.instance_group.worker_service_account_id,
                        )
                    except InstanceGroupNotFoundError:
                        self.logger.error(f'Group {instance_group_id} was not found during deletion')
