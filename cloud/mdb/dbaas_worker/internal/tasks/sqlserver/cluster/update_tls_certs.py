"""
Update SQLServer cluster TLS certs executor
"""

from ...common.cluster.create import ClusterCreateExecutor
from ...utils import build_host_group, register_executor
from ..utils import classify_host_map


@register_executor('sqlserver_cluster_update_tls_certs')
class SQLServerClusterUpdateTlsCerts(ClusterCreateExecutor):
    """
    Update TLS certs in SQLServer cluster
    """

    def run(self):
        self.mlock.lock_cluster(sorted(self.args['hosts']))
        hosts, _ = classify_host_map(self.args['hosts'])
        host_group = build_host_group(self.config.sqlserver, hosts)
        force_tls_certs = self.args.get('force_tls_certs', False)
        self._issue_tls(host_group, True, force_tls_certs=force_tls_certs)
        self._highstate_host_group(host_group)
        self.mlock.unlock_cluster()
        # Restart services if needed
        if self.args.get('restart', False):
            self._health_host_group(host_group, '-pre-restart')
            # Should we put cluster in some maintenance mode before restart?
            self._run_operation_host_group(
                host_group,
                'service',
                title='post-tls-restart',
                pillar={'service-restart': True},
                # I'm not sure how we need to order hosts in SQL server, so just get random order
                order=host_group.hosts.keys(),
            )
