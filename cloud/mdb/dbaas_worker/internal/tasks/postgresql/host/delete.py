"""
PostgreSQL Delete host executor
"""

import os

from ....providers.pgsync import PgSync, pgsync_cluster_prefix
from ....providers.zookeeper import Zookeeper
from ...common.host.delete import HostDeleteExecutor
from ...utils import build_host_group, register_executor


@register_executor('postgresql_host_delete')
class PostgreSQLHostDelete(HostDeleteExecutor):
    """
    Delete postgresql host
    """

    def __init__(self, config, task, queue, args):
        super().__init__(config, task, queue, args)
        self.pgsync = PgSync(config, task, queue)
        self.zookeeper = Zookeeper(config, task, queue)
        self.properties = config.postgresql

    def run(self):
        lock_hosts = list(self.args['hosts'])
        lock_hosts.append(self.args['host']['fqdn'])
        self.mlock.lock_cluster(sorted(lock_hosts))
        host_group = build_host_group(self.config.postgresql, self.args['hosts'])
        self._health_host_group(host_group)

        self.pgsync.ensure_replica(
            self.args['host']['fqdn'], self.args['zk_hosts'], pgsync_cluster_prefix(self.task['cid'])
        )
        self._delete_host()

        self._run_operation_host_group(host_group, 'metadata')

        self.zookeeper.absent(
            self.args['zk_hosts'], os.path.join('/pgsync', self.task['cid'], 'all_hosts', self.args['host']['fqdn'])
        )
        self.mlock.unlock_cluster()
