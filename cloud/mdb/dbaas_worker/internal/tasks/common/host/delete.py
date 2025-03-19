"""
Common host delete executor.
"""

from ...utils import build_host_group, build_host_group_from_list
from ..delete import BaseDeleteExecutor


class HostDeleteExecutor(BaseDeleteExecutor):
    """
    Generic class for host delete executors
    """

    def __init__(self, config, task, queue, args):
        super().__init__(config, task, queue, args)
        self.properties = None

    def _delete_host(self):
        """
        Delete host from dbm/compute, dns, conductor, deploy api, and certificator
        """
        host_group = build_host_group(self.properties, self.args['hosts'])
        self._health_host_group(host_group)
        delete_host_group = build_host_group_from_list(self.properties, [self.args['host']])
        self._delete_host_group_full(delete_host_group)

    def run(self):
        lock_hosts = list(self.args['hosts'])
        lock_hosts.append(self.args['host']['fqdn'])
        self.mlock.lock_cluster(sorted(lock_hosts))
        host_group = build_host_group(self.properties, self.args['hosts'])
        self._run_operation_host_group(host_group, 'metadata')
        self._delete_host()
        self.mlock.unlock_cluster()
