"""
Update Greenplum cluster TLS certs executor
"""

from ...common.cluster.create import ClusterCreateExecutor
from ...utils import register_executor
from ..utils import host_groups


@register_executor('greenplum_cluster_update_tls_certs')
class GreenplumClusterUpdateTlsCerts(ClusterCreateExecutor):
    """
    Update TLS certs in Greenplum cluster
    """

    def __init__(self, config, task, queue, args):
        super().__init__(config, task, queue, args)
        self.properties = self.config.greenplum_master

    def run(self):
        self.mlock.lock_cluster(sorted(self.args['hosts']))

        master_host_group, segment_host_group = host_groups(
            self.args['hosts'], self.config.greenplum_master, self.config.greenplum_segment
        )

        force_tls_certs = self.args.get('force_tls_certs', False)

        self._issue_tls(master_host_group, True, force_tls_certs=force_tls_certs)
        self._issue_tls(segment_host_group, True, force_tls_certs=force_tls_certs)

        self._highstate_host_group(segment_host_group, title='update-tls')
        self._highstate_host_group(master_host_group, title='update-tls')

        if self.args.get('restart', False):
            self._health_host_group(segment_host_group, '-pre-restart')
            self._health_host_group(master_host_group, '-pre-restart')
            self._run_operation_host_group(
                master_host_group,
                'service',
                title='post-tls-restart',
                pillar={'service-restart': True},
                order=master_host_group.hosts.keys(),
            )

        self.mlock.unlock_cluster()
