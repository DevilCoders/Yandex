"""
Hadoop Stop cluster executor
"""
from typing import Dict

from cloud.mdb.dbaas_worker.internal.providers.dataproc_manager import ClusterNotHealthyError, DataprocManager
from cloud.mdb.dbaas_worker.internal.providers.user_compute import UserComputeApi
from cloud.mdb.dbaas_worker.internal.providers.instance_group import InstanceGroup
from cloud.mdb.dbaas_worker.internal.tasks.common.cluster.stop import ClusterStopExecutor
from cloud.mdb.dbaas_worker.internal.tasks.utils import build_host_group, register_executor
from cloud.mdb.dbaas_worker.internal.tasks.hadoop.utils import MASTER_ROLE_TYPE
from cloud.mdb.internal.python.compute.instances import InstanceStatus


@register_executor('hadoop_cluster_stop')
class HadoopClusterStop(ClusterStopExecutor):
    """
    Stop hadoop cluster in compute
    """

    def __init__(self, config, task, queue, args):
        super().__init__(config, task, queue, args)
        self.user_compute_api = UserComputeApi(self.config, self.task, self.queue)
        self.dataproc_manager = DataprocManager(self.config, self.task, self.queue)
        self.instance_group = InstanceGroup(self.config, self.task, self.queue)

    def _wait_for_next_unmanaged_vm_to_stop(self, operation_ids_to_hosts):  # pylint: disable=invalid-name
        """
        Waits for any operation to be finished, removes it from the queue and finishes instance stop process
        """
        completed_operation_id = self.user_compute_api.wait_for_any_finished_operation(
            operation_ids_to_hosts, timeout=1800.0
        )
        host = operation_ids_to_hosts[completed_operation_id]
        del operation_ids_to_hosts[completed_operation_id]
        self._finish_unmanaged_vm_stopping(host)

    def _finish_unmanaged_vm_stopping(self, host):
        """
        Finishes instance stop process, eg. removes necessary dns records
        """
        self.user_compute_api.instance_wait(host, InstanceStatus.STOPPED)
        self.dns_api.set_records(host, [])

    def _stop_unmanaged_host_group(self, host_group):
        """
        Stop unmanaged compute vms inside user's folder
        """
        operation_ids_to_hosts: Dict[str, str] = {}
        for host, opts in host_group.hosts.items():
            if len(operation_ids_to_hosts) == self.config.hadoop.running_operations_limit:
                self._wait_for_next_unmanaged_vm_to_stop(operation_ids_to_hosts)

            if opts['vtype'] == 'compute':
                if not self.user_compute_api.get_instance(fqdn=host, instance_id=opts['vtype_id']):
                    self.logger.info('Instance %s is absent, skipping', host)
                    continue
                operation_id = self.user_compute_api.instance_stopped(
                    fqdn=host,
                    instance_id=opts['vtype_id'],
                    referrer_id=opts['subcid'],
                )
                if operation_id:
                    operation_ids_to_hosts[operation_id] = host
                else:
                    self._finish_unmanaged_vm_stopping(host)
            else:
                raise RuntimeError('Unknown vtype: {vtype} for host {host}'.format(vtype=opts['vtype'], host=host))

        while operation_ids_to_hosts:
            self._wait_for_next_unmanaged_vm_to_stop(operation_ids_to_hosts)

    def run(self):
        host_group = build_host_group(self.config.hadoop, self.args['hosts'])
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
                    statuses=self.dataproc_manager.DECOMMISSIONED,
                    timeout=decommission_timeout,
                    service=self.dataproc_manager.YARN,
                )
            except ClusterNotHealthyError:
                self.logger.exception('Decommission did not finish until timeout')
        self._stop_unmanaged_host_group(host_group)
        if 'instance_group_ids' in self.args:
            for instance_group_id in self.args['instance_group_ids']:
                self.instance_group.work_as_worker_service_account()
                subcid = self.instance_group.metadb_instance_group.get_subcid_by_instance_group_id(
                    instance_group_id=instance_group_id,
                )
                self.instance_group.stop(instance_group_id, referrer_id=subcid)
