"""
Kafka topic delete executor
"""
from ....common.deploy import BaseDeployExecutor
from ....utils import register_executor


@register_executor('kafka_topic_delete')
class KafkaTopicDelete(BaseDeployExecutor):
    """
    Delete Kafka topic
    """

    def run(self):
        self.mlock.lock_cluster(sorted(self.args['hosts']))
        self.deploy_api.wait(
            [
                self._run_operation_host(
                    host,
                    'topic-delete',
                    self.args['hosts'][host]['environment'],
                    pillar={'target-topic': self.args['target-topic']},
                )
                for host in self.args['hosts']
            ]
        )
        self.mlock.unlock_cluster()
