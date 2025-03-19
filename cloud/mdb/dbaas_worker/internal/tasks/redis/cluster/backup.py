"""
Redis Create backup executor
"""
from typing import List

from ....providers.host_health import HostNotHealthyError
from ...common.cluster.backup import CreateBackupExecutor
from ...utils import build_host_group, choose_sharded_backup_hosts, register_executor


@register_executor('redis_cluster_create_backup')
class RedisClusterCreateBackup(CreateBackupExecutor):
    """
    Create Redis cluster backup
    """

    def _choose_backup_hosts(self) -> List[str]:
        return choose_sharded_backup_hosts(self.args['hosts'], self._is_redis_host_healthy)

    def _is_redis_host_healthy(self, host: str, opts: dict) -> bool:
        try:
            host_group = build_host_group(self.config.redis, {host: opts})
            self._health_host_group(host_group)
            return True
        except HostNotHealthyError:
            return False
