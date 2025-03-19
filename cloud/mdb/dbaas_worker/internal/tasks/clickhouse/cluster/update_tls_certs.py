"""
Update ClickHouse cluster TLS certs executor
"""

from ...common.cluster.create import ClusterCreateExecutor
from ...utils import build_host_group, register_executor
from ..utils import classify_host_map


@register_executor('clickhouse_cluster_update_tls_certs')
class ClickhouseClusterUpdateTlsCerts(ClusterCreateExecutor):
    """
    Update TLS certs in ClickHouse cluster
    """

    def run(self):
        self.mlock.lock_cluster(sorted(self.args['hosts']))
        ch_hosts, zk_hosts = classify_host_map(self.args['hosts'])
        host_group = build_host_group(self.config.clickhouse, ch_hosts)
        zk_group = build_host_group(self.config.zookeeper, zk_hosts)
        force_tls_certs = self.args.get('force_tls_certs', False)

        self._issue_tls(host_group, True, force_tls_certs=force_tls_certs)
        self._issue_tls(zk_group, True, force_tls_certs=force_tls_certs)

        self._highstate_host_group(host_group, title='update-tls')
        self._highstate_host_group(zk_group, title='update-tls')

        # Restart services if needed
        if self.args.get('restart', False):
            self._health_host_group(host_group, '-pre-restart')
            self._run_operation_host_group(
                host_group,
                'service',
                title='post-tls-restart',
                pillar={'service-restart': True},
                # We don't really care about order, we just need to do in in sequence
                order=host_group.hosts.keys(),
            )

        if self.args.get('restart_zk', False):
            self._health_host_group(zk_group, '-pre-restart')
            self._run_operation_host_group(
                zk_group,
                'service',
                title='post-tls-restart',
                pillar={'service-restart': True},
                # We don't really care about order, we just need to do in in sequence
                order=zk_group.hosts.keys(),
            )

        self.mlock.unlock_cluster()
