"""
ClickHouse ML model delete executor.
"""
from .....utils import get_first_key
from ....common.deploy import BaseDeployExecutor
from ....utils import register_executor, split_hosts_for_modify_by_shard_batches
from ...utils import classify_host_map, remove_user_object_from_cache


@register_executor('clickhouse_model_delete')
class ClickHouseModelDeleteExecutor(BaseDeployExecutor):
    """
    Delete ClickHouse ML model.
    """

    def run(self):
        model_name = self.args["target-model"]
        self.mlock.lock_cluster(sorted(self.args['hosts']))
        ch_hosts, _ = classify_host_map(self.args['hosts'])

        selected_ch_host = get_first_key(ch_hosts)
        remove_user_object_from_cache(self.deploy_api, selected_ch_host, 'ml_model', model_name)

        for i_hosts in split_hosts_for_modify_by_shard_batches(ch_hosts, fast_mode=self.args.get('fast_mode', False)):
            self.deploy_api.wait(
                [
                    self._run_operation_host(
                        host,
                        'model-delete',
                        i_hosts[host]['environment'],
                        pillar={'target-model': model_name},
                    )
                    for host in i_hosts
                ]
            )
        self.mlock.unlock_cluster()
