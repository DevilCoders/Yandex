"""
Common cluster delete executor.
"""

from ..delete import BaseDeleteExecutor
from ...utils import build_host_group


class ClusterDeleteExecutor(BaseDeleteExecutor):
    """
    Generic class for cluster delete executors
    """

    def __init__(self, config, task, queue, args):
        super().__init__(config, task, queue, args)
        self.properties = None
        self.restartable = True

    def _delete_hosts(self):
        """
        Delete hosts from dbm/compute
        """
        host_group = build_host_group(self.properties, self.args['hosts'])
        self._delete_host_group_minimal(host_group)
        self._delete_alerts(self.task['cid'])

    def run(self):
        self.mlock.lock_cluster(sorted(self.args['hosts']))
        self._delete_hosts()
        if self.use_sg:
            self.vpc_sg.delete_service_security_group(self.task['cid'])
        self.mlock.unlock_cluster()
