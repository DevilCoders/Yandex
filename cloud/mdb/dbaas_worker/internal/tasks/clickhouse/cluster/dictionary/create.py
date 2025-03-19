"""
ClickHouse Dictionary create executor
"""

from ....common.deploy import BaseDeployExecutor
from ....utils import register_executor


@register_executor('clickhouse_dictionary_create')
class ClickHouseDictionaryCreateExecutor(BaseDeployExecutor):
    """
    Create clickhouse dictionary
    """

    def run(self):
        self.mlock.lock_cluster(sorted(self.args['hosts']))
        self.deploy_api.wait(
            [
                self._run_operation_host(
                    host,
                    'dictionary-create',
                    self.args['hosts'][host]['environment'],
                    pillar={'target-dictionary': self.args['target-dictionary']},
                )
                for host in self.args['hosts']
            ]
        )
        self.mlock.unlock_cluster()
