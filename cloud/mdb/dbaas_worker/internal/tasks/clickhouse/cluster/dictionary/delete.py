"""
ClickHouse Dictionary delete executor
"""

from ....common.deploy import BaseDeployExecutor
from ....utils import register_executor, split_hosts_for_modify_by_shard_batches


@register_executor('clickhouse_dictionary_delete')
class ClickHouseDictionaryDeleteExecutor(BaseDeployExecutor):
    """
    Delete ClickHouse dictionary
    """

    def run(self):
        self.mlock.lock_cluster(sorted(self.args['hosts']))
        for i_hosts in split_hosts_for_modify_by_shard_batches(
            self.args['hosts'], fast_mode=self.args.get('fast_mode', False)
        ):
            self.deploy_api.wait(
                [
                    self._run_operation_host(
                        host,
                        'dictionary-delete',
                        self.args['hosts'][host]['environment'],
                        pillar={'target-dictionary': self.args['target-dictionary']},
                    )
                    for host in i_hosts
                ]
            )
        self.mlock.unlock_cluster()
