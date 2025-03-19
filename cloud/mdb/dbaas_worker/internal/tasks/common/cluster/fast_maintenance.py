"""
Common cluster maintenance executor.
"""
from .maintenance import ClusterMaintenanceExecutor
from ...utils import Downtime


class ClusterFastMaintenanceExecutor(ClusterMaintenanceExecutor):
    """
    Base class for cluster fast maintenance executors
    """

    def __init__(self, config, task, queue, args):
        super().__init__(config, task, queue, args)

    def _run_fast_maintenance_task(self, host_group, get_order, disable_monitoring=False, components_for_update=None):
        dbaas_operations = []
        if components_for_update:
            dbaas_operations.append('update-components')
        else:
            components_for_update = []

        if self.args.get('update_tls', False):
            force_tls_certs = self.args.get('force_tls_certs', False)
            self._issue_tls(host_group, force_tls_certs=force_tls_certs)
            dbaas_operations.append('update-tls')

        for host in host_group.hosts:
            host_group.hosts[host]['deploy'] = {'pillar': {}}
            for operation in components_for_update:
                host_group.hosts[host]['deploy']['pillar'].update({operation: True})

        if len(dbaas_operations) == 0:
            self.logger.error("There is no implemented operations for fast maintenance task")
            return

        order = get_order()
        if disable_monitoring:
            with Downtime(self.juggler, host_group, self.task['timeout']):
                for dbaas_operation in dbaas_operations:
                    self._run_operation_host_group(host_group, dbaas_operation, order=order, title=dbaas_operation)
        else:
            for dbaas_operation in dbaas_operations:
                self._run_operation_host_group(host_group, dbaas_operation, order=order, title=dbaas_operation)
