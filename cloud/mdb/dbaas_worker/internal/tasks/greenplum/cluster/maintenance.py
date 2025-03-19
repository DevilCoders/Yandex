"""
Greenplum cluster maintenance executor
"""

from ...common.cluster.maintenance import ClusterMaintenanceExecutor
from ...utils import register_executor
from ..utils import host_groups


@register_executor('greenplum_cluster_maintenance')
class GreenplumClusterMaintenance(ClusterMaintenanceExecutor):
    """
    Perform Maintenance on Greenplum cluster
    """

    def __init__(self, config, task, queue, args):
        super().__init__(config, task, queue, args)
        self.properties = self.config.greenplum_master

    def run(self):
        self.mlock.lock_cluster(sorted(self.args['hosts']))

        master_host_group, segment_host_group = host_groups(
            self.args['hosts'], self.config.greenplum_master, self.config.greenplum_segment
        )
        if self.args.get('update_tls', False):
            force_tls_certs = self.args.get('force_tls_certs', False)
            self._issue_tls(master_host_group, force_tls_certs=force_tls_certs)
            self._issue_tls(segment_host_group, force_tls_certs=force_tls_certs)

        if self._is_offline_maintenance():
            # Sometimes too old hosts can't start without highstate(i.e. expired cert), so omit checks here.
            if 'disable_health_check' not in self.args:
                self.args['disable_health_check'] = True
            self.args['update_tls'] = True
        self._start_offline_maintenance(segment_host_group)
        self._start_offline_maintenance(master_host_group)

        self._highstate_host_group(segment_host_group, title='maintenance-hs')
        self._highstate_host_group(master_host_group, title='maintenance-hs')

        if self.args.get('restart', False):
            self._health_host_group(segment_host_group, '-pre-restart')
            self._health_host_group(master_host_group, '-pre-restart')
            self._run_operation_host_group(
                master_host_group,
                'service',
                title='post-restart',
                pillar={'service-restart': True},
                order=master_host_group.hosts.keys(),
            )
        self._stop_offline_maintenance(master_host_group)
        self._stop_offline_maintenance(segment_host_group)

        self.mlock.unlock_cluster()
