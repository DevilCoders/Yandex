"""
Kafka maintenance cluster executor
"""

from cloud.mdb.dbaas_worker.internal.providers.pillar import DbaasPillar
from cloud.mdb.dbaas_worker.internal.tasks.common.cluster.maintenance import ClusterMaintenanceExecutor
from cloud.mdb.dbaas_worker.internal.tasks.utils import Downtime, register_executor
from cloud.mdb.dbaas_worker.internal.tasks.kafka.utils import build_kafka_host_groups


@register_executor('kafka_cluster_maintenance')
class KafkaClusterMaintenance(ClusterMaintenanceExecutor):
    """
    Maintenance Kafka cluster
    """

    def __init__(self, config, task, queue, args):
        super().__init__(config, task, queue, args)
        self.pillar = DbaasPillar(config, task, queue)

    def run(self):
        if self.args.get('enable_plain_sasl', False):
            return self.enable_plain_sasl()

        self.mlock.lock_cluster(sorted(self.args['hosts']))
        kafka_group, zookeeper_group = build_kafka_host_groups(
            self.args['hosts'],
            self.config.kafka,
            self.config.zookeeper,
        )
        self._start_offline_maintenance(zookeeper_group)
        self._start_offline_maintenance(kafka_group)

        self._health_host_group(kafka_group)
        self._health_host_group(zookeeper_group)

        self._general_maintenance(kafka_group, zookeeper_group)

        self._stop_offline_maintenance(kafka_group)
        self._stop_offline_maintenance(zookeeper_group)
        self.mlock.unlock_cluster()

    def _general_maintenance(self, kafka_group, zookeeper_group):
        # Issue new TLS certificates if required
        if self.args.get('update_tls', False):
            force_tls_certs = self.args.get('force_tls_certs', False)
            self._issue_tls(kafka_group, force_tls_certs=force_tls_certs)
            self._issue_tls(zookeeper_group, force_tls_certs=force_tls_certs)

        self._highstate_host_group(zookeeper_group, title="hs-zk")
        if self.args.get('restart_zk', False):
            self._run_operation_host_group(
                zookeeper_group,
                'service',
                title='restart-zk',
                pillar={'service-restart': True},
                order=zookeeper_group.hosts.keys(),
            )

        self._highstate_host_group(kafka_group, title="hs-kafka")
        if self.args.get('restart', False):
            with Downtime(self.juggler, kafka_group, self.task['timeout']):
                order = sorted(kafka_group.hosts.keys())
                self._run_operation_host_group(
                    kafka_group,
                    'service',
                    title='restart-kafka',
                    pillar={'service-restart': True},
                    order=order,
                )

    def enable_plain_sasl(self):
        self.mlock.lock_cluster(sorted(self.args['hosts']))
        kafka_group, zk_group = build_kafka_host_groups(self.args['hosts'], self.config.kafka, self.config.zookeeper)
        self._start_offline_maintenance(zk_group)
        self._start_offline_maintenance(kafka_group)

        self._health_host_group(kafka_group)
        # run highstate to ensure that `sasl.enabled.mechanisms=SCRAM-SHA-512,PLAIN` is set within `server.properties`
        self.deploy_api.wait(
            [self.deploy_api.run(host, deploy_title='first-hs') for host, opts in kafka_group.hosts.items()]
        )
        # restart brokers so that setting `sasl.enabled.mechanisms=SCRAM-SHA-512,PLAIN` is actually enabled
        self._restart_brokers(kafka_group, 'first-kafka-restart')

        self.pillar.exists(
            'cid',
            self.task['cid'],
            ['data', 'kafka'],
            ['use_plain_sasl'],
            [True],
            force=True,
        )

        subcid = next(iter(kafka_group.hosts.values()))['subcid']
        self.pillar.remove(
            'subcid',
            subcid,
            ['data', 'kafka'],
            ['old_jmx_user'],
        )

        # run highstate to ensure that `server.properties` and some other places are switched to using PLAIN sasl
        self.deploy_api.wait(
            [self.deploy_api.run(host, deploy_title='second-hs') for host, opts in kafka_group.hosts.items()]
        )
        # restart brokers so that setting `sasl.mechanism.inter.broker.protocol=PLAIN` is actually enabled
        self._restart_brokers(kafka_group, 'second-kafka-restart')
        self._health_host_group(kafka_group, '-post-upgrade')

        self._stop_offline_maintenance(kafka_group)
        self._stop_offline_maintenance(zk_group)
        self.mlock.unlock_cluster()

    def _restart_brokers(self, kafka_group, deploy_title):
        order = sorted(kafka_group.hosts.keys())
        self._run_operation_host_group(
            kafka_group,
            'service',
            title=deploy_title,
            pillar={'service-restart': True},
            order=order,
        )
