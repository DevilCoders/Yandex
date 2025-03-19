"""
Apache Kafka host delete executor.
"""

from ...common.host.delete import HostDeleteExecutor
from ...utils import build_host_group_from_list, register_executor
from ..utils import build_kafka_host_groups


@register_executor('kafka_host_delete')
class KafkaHostDelete(HostDeleteExecutor):
    """
    Delete Kafka host in dbm or compute.

    It involves configuration update (invoking high-state) on all
    hosts except the deleting one. The update is performed before destroying
    the host.
    """

    def __init__(self, config, task, queue, args):
        super().__init__(config, task, queue, args)

    def run(self):
        lock_hosts = list(self.args['hosts'])
        lock_hosts.append(self.args['host']['fqdn'])
        self.mlock.lock_cluster(sorted(lock_hosts))

        kafka_group, zookeeper_group = build_kafka_host_groups(
            hosts=self.args['hosts'],
            cfg_kafka=self.config.kafka,
            cfg_zk=self.config.zookeeper,
        )

        self._run_operation_host_group(kafka_group, 'metadata')
        self._run_operation_host_group(zookeeper_group, 'metadata')

        delete_host_group = build_host_group_from_list(
            properties=self.config.kafka,
            hosts=[self.args['host']],
        )
        self._delete_host_group_full(delete_host_group)

        self.mlock.unlock_cluster()
