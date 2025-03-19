"""
ClickHouse Create backup executor
"""
from typing import List

from ....providers.common import Change
from ....providers.host_health import HostNotHealthyError
from ...common.cluster.backup import CreateBackupExecutor
from ...utils import build_host_group, choose_sharded_backup_hosts, register_executor
from ..utils import classify_host_map


@register_executor('clickhouse_cluster_create_backup')
class ClickHouseClusterCreateBackup(CreateBackupExecutor):
    """
    Create ClickHouse cluster backup.

    Backup is executed on each shard and only on one host per shard.
    """

    def _choose_backup_hosts(self) -> List[str]:
        ch_hosts, _ = classify_host_map(self.args['hosts'], ignored_hosts=self.args.get('ignore_hosts', []))
        return choose_sharded_backup_hosts(ch_hosts, self._is_ch_host_healthy)

    def _is_ch_host_healthy(self, host: str, opts: dict) -> bool:
        try:
            host_group = build_host_group(self.config.clickhouse, {host: opts})
            self._health_host_group(host_group)
            return True
        except HostNotHealthyError:
            return False

    def run(self):
        self.mlock.lock_cluster(sorted(self.args['hosts']))
        backup_hosts = self._choose_backup_hosts()
        self.logger.info('Starting backup on %r', backup_hosts)
        self.deploy_api.wait(
            [
                self._run_operation_host(
                    host,
                    'backup',
                    self.args['hosts'][host]['environment'],
                    rollback=Change.noop_rollback,
                    pillar={"labels": self.args.get("labels", {})},
                )
                for host in backup_hosts
            ]
        )
        self.mlock.unlock_cluster()
