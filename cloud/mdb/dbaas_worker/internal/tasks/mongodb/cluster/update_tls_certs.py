"""
Update MongoDB cluster TLS certs executor
"""

from ...common.cluster.create import ClusterCreateExecutor
from ...utils import build_host_group, register_executor
from ..utils import classify_host_map


@register_executor('mongodb_cluster_update_tls_certs')
class MongodbClusterUpdateTlsCerts(ClusterCreateExecutor):
    """
    Update TLS certs in Mongodb cluster
    """

    def run(self):
        self.mlock.lock_cluster(sorted(self.args['hosts']))
        host_types = classify_host_map(self.args['hosts'])
        force_tls_certs = self.args.get('force_tls_certs', False)
        for host_type, hosts in host_types.items():
            host_group = build_host_group(getattr(self.config, host_type), hosts)
            self._issue_tls(host_group, True, force_tls_certs=force_tls_certs)
            self._highstate_host_group(host_group, title="update-tls")
        # Restart services if needed
        if self.args.get('restart', False):
            for host_type, hosts in host_types.items():
                host_group = build_host_group(getattr(self.config, host_type), hosts)
                self._health_host_group(host_group, f'-pre-restart-{host_type}')
                # We can restart MongoDB in parallel, salt states will do all work for
                # not restarting all host simultaniosly
                self._run_operation_host_group(
                    host_group,
                    'service',
                    title=f'post-tls-restart-{host_type}',
                    pillar={'service-restart': True},
                )
        self.mlock.unlock_cluster()
