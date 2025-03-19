"""
Kafka topic create executor
"""
from ....common.deploy import BaseDeployExecutor
from ....utils import register_executor
from ...utils import classify_host_map


@register_executor('kafka_topic_create')
class KafkaTopicCreate(BaseDeployExecutor):
    """
    Create Kafka topic
    """

    def run(self):
        kafka_hosts, _ = classify_host_map(self.args['hosts'])
        self.mlock.lock_cluster(sorted(self.args['hosts']))
        self.deploy_api.wait(
            [
                self._run_operation_host(
                    host,
                    'topic-create',
                    kafka_hosts[host]['environment'],
                    pillar={'target-topic': self.args['target-topic']},
                )
                for host in kafka_hosts
            ]
        )
        self.mlock.unlock_cluster()
