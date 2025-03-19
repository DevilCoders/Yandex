"""
Hadoop Start cluster executor
"""

import time
from typing import Dict

from cloud.mdb.dbaas_worker.internal.providers.dataproc_manager import DataprocManager
from cloud.mdb.dbaas_worker.internal.providers.dns import Record
from cloud.mdb.dbaas_worker.internal.providers.user_compute import UserComputeApi
from cloud.mdb.dbaas_worker.internal.providers.instance_group import InstanceGroup
from cloud.mdb.dbaas_worker.internal.tasks.common.cluster.start import ClusterStartExecutor
from cloud.mdb.dbaas_worker.internal.tasks.utils import build_host_group, register_executor
from cloud.mdb.dbaas_worker.internal.tasks.hadoop.constants import DATAPROC_MANAGER_FLAG


@register_executor('hadoop_cluster_start')
class HadoopClusterStart(ClusterStartExecutor):
    """
    Start hadoop cluster in compute
    """

    def __init__(self, config, task, queue, args):
        super().__init__(config, task, queue, args)
        self.dataproc_manager = DataprocManager(self.config, self.task, self.queue)
        self.user_compute_api = UserComputeApi(self.config, self.task, self.queue)
        self.instance_group = InstanceGroup(self.config, self.task, self.queue)

    def _wait_for_next_unmanaged_vm_to_start(self, operation_ids_to_hosts):  # pylint: disable=invalid-name
        """
        Waits for any operation to be finished, removes it from the queue and finishes instance start process
        """
        completed_operation_id = self.user_compute_api.wait_for_any_finished_operation(
            operation_ids_to_hosts, timeout=1800.0
        )
        host = operation_ids_to_hosts[completed_operation_id]
        del operation_ids_to_hosts[completed_operation_id]
        self._finish_unmanaged_vm_starting(host)

    def _finish_unmanaged_vm_starting(self, host):
        """
        Finishes instance start process, eg. creates necessary dns records
        """
        self.user_compute_api.instance_wait(host)
        self._create_unmanaged_record(host)

    def _start_unmanaged_host_group(self, host_group):
        """
        Start unmanaged hosts inside user's folder
        """

        operation_ids_to_hosts: Dict[str, str] = {}

        for host, opts in host_group.hosts.items():
            if len(operation_ids_to_hosts) == self.config.hadoop.running_operations_limit:
                self._wait_for_next_unmanaged_vm_to_start(operation_ids_to_hosts)

            if opts['vtype'] == 'compute':
                operation_id = self.user_compute_api.instance_running(
                    fqdn=host,
                    instance_id=opts['vtype_id'],
                    referrer_id=opts['subcid'],
                )
                if operation_id:
                    operation_ids_to_hosts[operation_id] = host
                else:
                    self._finish_unmanaged_vm_starting(host)
            else:
                raise RuntimeError('Unknown vtype: {vtype} for host {host}'.format(vtype=opts['vtype'], host=host))

        while operation_ids_to_hosts:
            self._wait_for_next_unmanaged_vm_to_start(operation_ids_to_hosts)

    def _create_unmanaged_record(self, host):
        """
        Create public dns records for unmanaged hosts
        """
        public_records = []
        for address, version in self.user_compute_api.get_instance_public_addresses(host):
            public_records.append(Record(address=address, record_type=('AAAA' if version == 6 else 'A')))
        self.dns_api.set_records(host, public_records)

    def run(self):
        host_group = build_host_group(self.config.hadoop, self.args['hosts'])
        self._start_unmanaged_host_group(host_group)
        if 'instance_group_ids' in self.args:
            for instance_group_id in self.args['instance_group_ids']:
                self.instance_group.work_as_worker_service_account()
                subcid = self.instance_group.metadb_instance_group.get_subcid_by_instance_group_id(
                    instance_group_id=instance_group_id,
                )
                self.instance_group.start(instance_group_id, referrer_id=subcid)
        if DATAPROC_MANAGER_FLAG in self.task['feature_flags']:
            finished_updating_compute_resources_at = int(time.time())
            self.dataproc_manager.cluster_wait(
                self.task['cid'], timeout=60 * 30, updated_after=finished_updating_compute_resources_at
            )
