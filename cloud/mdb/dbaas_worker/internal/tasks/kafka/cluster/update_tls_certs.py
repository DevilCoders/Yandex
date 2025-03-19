"""
Update Kafka cluster TLS certs executor
"""

from ...common.cluster.create import ClusterCreateExecutor
from ...utils import build_host_group, register_executor
from ..utils import classify_host_map, make_cluster_dns_record


@register_executor('kafka_cluster_update_tls_certs')
class KafkaClusterUpdateTlsCerts(ClusterCreateExecutor):
    """
    Update TLS certs in Kafka cluster
    """

    def run(self):
        self.mlock.lock_cluster(sorted(self.args['hosts']))
        kafka_hosts, zookeeper_hosts = classify_host_map(self.args['hosts'])
        kafka_host_group = build_host_group(self.config.kafka, kafka_hosts)
        zookeeper_host_group = build_host_group(self.config.zookeeper, zookeeper_hosts)
        force_tls_certs = self.args.get('force_tls_certs', False)

        self._issue_tls(kafka_host_group, True, force_tls_certs=force_tls_certs)
        self._issue_tls(zookeeper_host_group, True, force_tls_certs=force_tls_certs)

        make_cluster_dns_record(self, kafka_host_group)

        self._highstate_host_group(kafka_host_group, title='update-tls')
        self._highstate_host_group(zookeeper_host_group, title='update-tls')

        # Restart services if needed
        if self.args.get('restart', False):
            self._health_host_group(kafka_host_group, '-pre-restart')
            self._run_operation_host_group(
                kafka_host_group,
                'service',
                title='post-tls-restart',
                pillar={'service-restart': True},
                # We don't really care about order, we just need to do in in sequence
                order=kafka_host_group.hosts.keys(),
            )

        if self.args.get('restart_zk', False):
            self._health_host_group(zookeeper_host_group, '-pre-restart')
            self._run_operation_host_group(
                zookeeper_host_group,
                'service',
                title='post-tls-restart',
                pillar={'service-restart': True},
                # We don't really care about order, we just need to do in in sequence
                order=zookeeper_host_group.hosts.keys(),
            )

        self.mlock.unlock_cluster()
