"""
ClickHouse ML model modify executor.
"""
from .....utils import get_first_key
from ....common.deploy import BaseDeployExecutor
from ....utils import register_executor, split_hosts_for_modify_by_shard_batches
from ...utils import classify_host_map, cache_user_object


@register_executor('clickhouse_model_modify')
class ClickHouseModelModifyExecutor(BaseDeployExecutor):
    """
    Modify ClickHouse ML model.
    """

    def run(self):
        model_name = self.args["target-model"]
        self.mlock.lock_cluster(sorted(self.args['hosts']))
        ch_hosts, _ = classify_host_map(self.args['hosts'])

        selected_ch_host = get_first_key(ch_hosts)
        cache_user_object(
            self.deploy_api,
            selected_ch_host,
            ch_hosts[selected_ch_host]['environment'],
            'ml_model',
            model_name,
            remove_on_error=False,
        )

        for i_hosts in split_hosts_for_modify_by_shard_batches(ch_hosts, fast_mode=self.args.get('fast_mode', False)):
            self.deploy_api.wait(
                [
                    self._run_operation_host(
                        host,
                        'model-modify',
                        i_hosts[host]['environment'],
                        pillar={'target-model': model_name},
                    )
                    for host in i_hosts
                ]
            )
        self.mlock.unlock_cluster()
