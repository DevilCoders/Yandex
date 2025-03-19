"""
Kafka connector delete executor
"""
from cloud.mdb.dbaas_worker.internal.providers.common import Change
from ...utils import classify_host_map
from ....common.deploy import BaseDeployExecutor
from ....utils import register_executor


@register_executor('kafka_connector_delete')
class KafkaConnectorDelete(BaseDeployExecutor):
    """
    Delete Kafka connector
    """

    def run(self):
        kafka_hosts, _ = classify_host_map(self.args['hosts'])
        self.mlock.lock_cluster(sorted(kafka_hosts))
        self.deploy_api.wait(
            [
                self._run_operation_host(
                    host,
                    'connector-delete',
                    kafka_hosts[host]['environment'],
                    pillar={'target-connector': self.args['target-connector']},
                    rollback=Change.noop_rollback,
                )
                for host in kafka_hosts
            ]
        )
        self.mlock.unlock_cluster()
