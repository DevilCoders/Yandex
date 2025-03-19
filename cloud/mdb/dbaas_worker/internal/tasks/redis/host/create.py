"""
Redis Create host executor
"""

from ...common.host.create import HostCreateExecutor
from ...utils import register_executor


@register_executor('redis_host_create')
@register_executor('redis_shard_host_create')
class RedisHostCreate(HostCreateExecutor):
    """
    Create Redis host in dbm or compute
    """

    def __init__(self, config, task, queue, args):
        super().__init__(config, task, queue, args)
        self.properties = self.config.redis
