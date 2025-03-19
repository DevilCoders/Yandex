"""
Redis Modify host executor
"""
from ...common.host.modify import HostModifyExecutor
from ...utils import register_executor


@register_executor('redis_host_modify')
class RedisHostModify(HostModifyExecutor):
    """
    Modify Redis host
    """

    def __init__(self, config, task, queue, args):
        super().__init__(config, task, queue, args)
        self.properties = config.redis

    def run(self):
        self.mlock.lock_cluster(sorted(self.args['hosts']))

        target_host = self.args['host']['fqdn']
        self._modify_host(target_host, pillar={})

        self.mlock.unlock_cluster()
