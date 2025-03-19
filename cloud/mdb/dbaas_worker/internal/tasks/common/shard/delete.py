"""
Common shard delete executor.
"""

from ...utils import build_host_group_from_list
from ..delete import BaseDeleteExecutor


class ShardDeleteExecutor(BaseDeleteExecutor):
    """
    Generic class for shard delete executors
    """

    def __init__(self, config, task, queue, args):
        super().__init__(config, task, queue, args)
        self.properties = None

    def _delete_hosts(self):
        """
        Delete hosts from dbm/compute, dns, conductor, deploy api, and certificator
        """
        host_group = build_host_group_from_list(self.properties, self.args['shard_hosts'])
        self._delete_host_group_full(host_group)
