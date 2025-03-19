"""
Kafka connector create executor
"""
from ....common.deploy import BaseDeployExecutor
from ....utils import register_executor
from ...utils import classify_host_map
from cloud.mdb.dbaas_worker.internal.providers.deploy import DeployErrorMaxAttempts
from cloud.mdb.dbaas_worker.internal.exceptions import UserExposedException


@register_executor('kafka_connector_create')
class KafkaConnectorCreate(BaseDeployExecutor):
    """
    Create Kafka connector
    """

    def run(self):
        kafka_hosts, _ = classify_host_map(self.args['hosts'])

        def rollback(*_):
            self.deploy_api.wait(
                [
                    self._run_operation_host(
                        host,
                        'connector-delete',
                        kafka_hosts[host]['environment'],
                        pillar={'target-connector': self.args['target-connector']},
                    )
                    for host in kafka_hosts
                ]
            )

        self.mlock.lock_cluster(sorted(kafka_hosts))
        # Flag to check that task failed in highstate or not
        highstate_passed = False
        # Calling highstate with new pillar flag if it's the first connector
        if self.args.get('enable-connect'):

            def hs_rollback(*_):
                # If this flag is True, highstate complete without error and we can rollback changes of connector
                if not highstate_passed:
                    raise RuntimeError("Cannot rollback highstate error")

            self.deploy_api.wait(
                [
                    self.deploy_api.run(
                        host,
                        deploy_title='enable_kafka_connect',
                        rollback=hs_rollback,
                    )
                    for host in kafka_hosts
                ]
            )
        highstate_passed = True
        # Creating connector itself
        try:
            self.deploy_api.wait(
                [
                    self._run_operation_host(
                        host,
                        'connector-create',
                        kafka_hosts[host]['environment'],
                        pillar={'target-connector': self.args['target-connector']},
                        rollback=rollback,
                    )
                    for host in kafka_hosts
                ]
            )
        except DeployErrorMaxAttempts as err:
            for shipment in err.shipments:
                data = self.deploy_api.get_shipment_jobs(shipment.shipment.jid) or {}
                for job in data.get('jobs', []):
                    if job['status'] != 'error':
                        continue
                    job_results = self.deploy_api.get_job_results(shipment.shipment.fqdn, job['extId'])
                    for result in job_results['jobResults']:
                        if result['status'] != 'failure':
                            continue
                        result_data = result.get('result', {})
                        return_data = result_data.get('return', {})
                        if not isinstance(return_data, dict):
                            continue
                        for salt_return in return_data.values():
                            if salt_return.get('retcode') != 3:
                                continue
                            comment = salt_return.get('comment', '').strip()
                            if not comment.startswith('Validation error:'):
                                continue
                            raise UserExposedException(comment, err_type='Validation error')
            raise
        self.mlock.unlock_cluster()
