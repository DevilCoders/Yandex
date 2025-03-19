"""
Kafka Upgrade cluster executor.
"""

from cloud.mdb.dbaas_worker.internal.providers.base_metadb import execute
from cloud.mdb.dbaas_worker.internal.providers.pillar import DbaasPillar
from cloud.mdb.dbaas_worker.internal.tasks.common.cluster.modify import ClusterModifyExecutor
from cloud.mdb.dbaas_worker.internal.tasks.utils import register_executor
from cloud.mdb.dbaas_worker.internal.tasks.kafka.utils import build_kafka_host_groups


@register_executor('kafka_cluster_upgrade')
class KafkaClusterUpgrade(ClusterModifyExecutor):
    """
    Upgrade Kafka cluster
    """

    def __init__(self, config, task, queue, args):
        super().__init__(config, task, queue, args)
        self.pillar = DbaasPillar(config, task, queue)

    def run(self):
        self.mlock.lock_cluster(sorted(self.args['hosts']))
        kafka_group, zk_group = build_kafka_host_groups(self.args['hosts'], self.config.kafka, self.config.zookeeper)

        self._health_host_group(kafka_group)
        self._health_host_group(zk_group)

        kafka_pillar_data = self._get_kafka_pillar_data()
        target_version = kafka_pillar_data['version']

        if len(kafka_group.hosts) > 1:
            if kafka_pillar_data.get('inter_broker_protocol_version') != target_version:
                self._install_package_and_update_config(kafka_group)
                self._restart_brokers(kafka_group, 'first-kafka-restart')
                # check health before changing inter.broker.protocol.version
                self._health_host_group(
                    kafka_group, '-after-first-restart', timeout=self.deploy_api.get_seconds_to_deadline()
                )
                self._update_inter_broker_protocol_version(target_version)

            self._restart_brokers(kafka_group, 'second-kafka-restart')
        else:
            self._update_inter_broker_protocol_version(target_version)
            self._install_package_and_update_config(kafka_group)
            self._restart_brokers(kafka_group, 'single-kafka-restart')

        self._health_host_group(kafka_group, '-post-upgrade')

        self.mlock.unlock_cluster()

    def _get_kafka_pillar_data(self):
        with self.metadb.get_master_conn() as conn:
            with conn:
                cur = conn.cursor()
                res = execute(
                    cur,
                    'get_pillar',
                    path=['data', 'kafka'],
                    cid=self.task['cid'],
                    subcid=None,
                    fqdn=None,
                    shard_id=None,
                )
                return res[0]['value']

    def _restart_brokers(self, kafka_group, deploy_title):
        order = sorted(kafka_group.hosts.keys())
        self._run_operation_host_group(
            kafka_group,
            'service',
            title=deploy_title,
            pillar={'service-restart': True},
            order=order,
        )

    def _install_package_and_update_config(self, kafka_group):
        self.deploy_api.wait(
            [self.deploy_api.run(host, deploy_title='install-new-kafka') for host, opts in kafka_group.hosts.items()]
        )

    def _update_inter_broker_protocol_version(self, target_version):
        self.pillar.exists(
            'cid',
            self.task['cid'],
            ['data', 'kafka'],
            ['inter_broker_protocol_version'],
            [target_version],
            force=True,
        )
