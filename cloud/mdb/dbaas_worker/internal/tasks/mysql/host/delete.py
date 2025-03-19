"""
MySQL Delete host executor
"""

from ....providers.mysync import MySync
from ...common.host.delete import HostDeleteExecutor
from ...utils import build_host_group, register_executor


@register_executor('mysql_host_delete')
class MySQLHostDelete(HostDeleteExecutor):
    """
    Delete mysql host
    """

    def __init__(self, config, task, queue, args):
        super().__init__(config, task, queue, args)
        self.mysync = MySync(config, task, queue)
        self.properties = config.mysql

    def run(self):
        lock_hosts = list(self.args['hosts'])
        lock_hosts.append(self.args['host']['fqdn'])
        self.mlock.lock_cluster(sorted(lock_hosts))
        host_group = build_host_group(self.config.mysql, self.args['hosts'])
        self._health_host_group(host_group)
        self.mysync.ensure_replica(self.args['zk_hosts'], self.task['cid'], self.args['host']['fqdn'])
        self._run_operation_host_group(host_group, 'metadata')
        self._delete_host()
        self.mysync.ha_host_absent(self.args['zk_hosts'], self.task['cid'], self.args['host']['fqdn'])
        self.mysync.cascade_host_absent(self.args['zk_hosts'], self.task['cid'], self.args['host']['fqdn'])
        self.mlock.unlock_cluster()
