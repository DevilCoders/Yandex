"""
ClickHouse format schema delete executor.
"""
from .....utils import get_first_key
from ....common.deploy import BaseDeployExecutor
from ....utils import register_executor
from ...utils import classify_host_map, remove_user_object_from_cache


@register_executor('clickhouse_format_schema_delete')
class ClickHouseFormatSchemaDeleteExecutor(BaseDeployExecutor):
    """
    Delete ClickHouse format schema.
    """

    def run(self):
        format_schema_name = self.args["target-format-schema"]
        self.mlock.lock_cluster(sorted(self.args['hosts']))
        ch_hosts, _ = classify_host_map(self.args['hosts'])

        selected_ch_host = get_first_key(ch_hosts)
        remove_user_object_from_cache(self.deploy_api, selected_ch_host, 'format_schema', format_schema_name)

        self.deploy_api.wait(
            [
                self._run_operation_host(
                    host,
                    'format-schema-delete',
                    ch_hosts[host]['environment'],
                    pillar={'target-format-schema': self.args['target-format-schema']},
                )
                for host in ch_hosts
            ]
        )
        self.mlock.unlock_cluster()
