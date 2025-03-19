"""
Kafka Create host executor
"""
from copy import deepcopy

from ...common.host.create import HostCreateExecutor
from ...utils import Downtime, register_executor
from ..utils import classify_host_map
from ..utils import build_kafka_host_groups


@register_executor('kafka_host_create')
class KafkaHostCreate(HostCreateExecutor):
    """
    Create kafka host in dbm or compute
    """

    def __init__(self, config, task, queue, args):
        super().__init__(config, task, queue, args)
        self.properties = self.config.kafka
        kafka_hosts, _ = classify_host_map(self.args['hosts'])
        self.properties.conductor_group_id = next(iter(kafka_hosts.values()))['subcid']


@register_executor('kafka_zookeeper_host_create')
class KafkaZookeeperHostCreate(HostCreateExecutor):
    """
    Create Zookeeper host in dbm or compute.
    """

    def __init__(self, config, task, queue, args):
        super().__init__(config, task, queue, args)
        self.properties = deepcopy(self.config.zookeeper)
        _, zk_hosts = classify_host_map(self.args['hosts'])
        self.properties.conductor_group_id = next(iter(zk_hosts.values()))['subcid']

    def run(self):
        self.acquire_lock()
        self._create_host()

        kafka_group, zookeeper_group = build_kafka_host_groups(
            self.args['hosts'],
            self.config.kafka,
            self.config.zookeeper,
        )

        # validate that the new node has joined the cluster
        self._health_host_group(zookeeper_group)
        self._highstate_host_group(zookeeper_group, title="hs-zk")
        self._run_operation_host_group(
            zookeeper_group,
            'service',
            title='restart-zk',
            pillar={'service-restart': True},
            order=zookeeper_group.hosts.keys(),
        )

        self._highstate_host_group(kafka_group, title="hs-kafka")
        with Downtime(self.juggler, kafka_group, self.task['timeout']):
            order = sorted(kafka_group.hosts.keys())
            self._run_operation_host_group(
                kafka_group,
                'service',
                title='restart-kafka',
                pillar={'service-restart': True},
                order=order,
            )

        self.release_lock()
