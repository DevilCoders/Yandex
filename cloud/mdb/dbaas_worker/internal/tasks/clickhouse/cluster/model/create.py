"""
ClickHouse ML model create executor.
"""
from .....utils import get_first_key
from ....common.deploy import BaseDeployExecutor
from ....utils import register_executor
from ...utils import classify_host_map, cache_user_object


@register_executor('clickhouse_model_create')
class ClickHouseModelCreateExecutor(BaseDeployExecutor):
    """
    Create ClickHouse ML model.
    """

    def run(self):
        model_name = self.args["target-model"]
        self.mlock.lock_cluster(sorted(self.args['hosts']))
        ch_hosts, _ = classify_host_map(self.args['hosts'])

        selected_ch_host = get_first_key(ch_hosts)
        cache_user_object(
            self.deploy_api, selected_ch_host, ch_hosts[selected_ch_host]['environment'], 'ml_model', model_name
        )

        self.deploy_api.wait(
            [
                self._run_operation_host(
                    host,
                    'model-create',
                    ch_hosts[host]['environment'],
                    pillar={'target-model': model_name},
                )
                for host in ch_hosts
            ]
        )
        self.mlock.unlock_cluster()
