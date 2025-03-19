"""
ClickHouse format schema modify executor.
"""
from .....utils import get_first_key
from ....common.deploy import BaseDeployExecutor
from ....utils import register_executor
from ...utils import classify_host_map, cache_user_object


@register_executor('clickhouse_format_schema_modify')
class ClickHouseFormatSchemaModifyExecutor(BaseDeployExecutor):
    """
    Modify ClickHouse format schema.
    """

    def run(self):
        format_schema_name = self.args["target-format-schema"]
        self.mlock.lock_cluster(sorted(self.args['hosts']))
        ch_hosts, _ = classify_host_map(self.args['hosts'])

        selected_ch_host = get_first_key(ch_hosts)
        cache_user_object(
            self.deploy_api,
            selected_ch_host,
            ch_hosts[selected_ch_host]['environment'],
            'format_schema',
            format_schema_name,
            remove_on_error=False,
        )

        self.deploy_api.wait(
            [
                self._run_operation_host(
                    host,
                    'format-schema-create',
                    ch_hosts[host]['environment'],
                    pillar={'target-format-schema': format_schema_name},
                )
                for host in ch_hosts
            ]
        )
        self.mlock.unlock_cluster()
