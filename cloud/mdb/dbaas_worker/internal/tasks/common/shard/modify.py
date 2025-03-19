"""
Common shard modify executor.
"""
from ..modify import BaseModifyExecutor


class ShardModifyExecutor(BaseModifyExecutor):
    """
    Generic class for shard modify executors.
    """

    def _modify_shard(self, shard_host_group, shard_order, rest_host_groups):
        """
        Run modify on shard hosts and then on rest hosts
        """
        if self.args.get('reverse_order'):
            self._run_operation_host_group(shard_host_group, 'service', order=shard_order)

        self._change_host_group(shard_host_group)

        if not self.args.get('reverse_order'):
            self._run_operation_host_group(shard_host_group, 'service', order=shard_order)

        for group in rest_host_groups:
            self._run_operation_host_group(group, 'metadata')
