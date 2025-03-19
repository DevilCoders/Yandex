"""
MongoDB Create backup executor
"""
from typing import List

from ....providers.host_health import HostNotHealthyError
from ...common.cluster.backup import CreateBackupExecutor
from ...utils import build_host_group, choose_sharded_backup_hosts, register_executor
from ..utils import MONGOD_HOST_TYPE, classify_host_map


@register_executor('mongodb_cluster_create_backup')
class MongodbClusterCreateBackup(CreateBackupExecutor):
    """
    Create MongoDB cluster backup

    Backup is executed on each shard and only on one host per shard.
    """

    def _choose_backup_hosts(self) -> List[str]:
        hosts = classify_host_map(self.args['hosts'])
        return choose_sharded_backup_hosts(hosts[MONGOD_HOST_TYPE], self._is_mongod_host_healthy)

    def _is_mongod_host_healthy(self, host: str, opts: dict) -> bool:
        try:
            host_group = build_host_group(getattr(self.config, MONGOD_HOST_TYPE), {host: opts})
            self._health_host_group(host_group)
            return True
        except HostNotHealthyError:
            return False
