"""
PostgreSQL cluster maintenance executor
"""

from .maintenance import PostgreSQLClusterMaintenance
from ...utils import register_executor


@register_executor('postgresql_cluster_fast_maintenance')
class PostgreSQLClusterFastMaintenance(PostgreSQLClusterMaintenance):
    """
    Perform Maintenance on PostgreSQL cluster
    """

    def __init__(self, config, task, queue, args):
        super().__init__(config, task, queue, args)

    def run(self, task=None):
        operations = []
        _disable_monitoring = False
        if self.args.get('update_odyssey', False):
            operations.append('update-odyssey')
            _disable_monitoring = True

        if self.args.get('update_postgresql', False):
            operations.append('update-postgresql')
            _disable_monitoring = True

        def task(host_group, get_order, disable_monitoring=False):
            self._run_fast_maintenance_task(
                host_group, get_order, disable_monitoring=disable_monitoring, components_for_update=operations
            )

        self._run(task, disable_monitoring=_disable_monitoring)
