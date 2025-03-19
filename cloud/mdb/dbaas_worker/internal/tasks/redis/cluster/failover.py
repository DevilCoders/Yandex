"""
Redis start cluster failover executor
"""
from ....utils import get_first_key
from ...common.deploy import BaseDeployExecutor
from ...utils import build_host_group, register_executor
from ....providers.common import Change


@register_executor('redis_cluster_start_failover')
class RedisStartClusterFailover(BaseDeployExecutor):
    """
    Start failover on Redis cluster
    """

    def __init__(self, config, task, queue, args):
        super().__init__(config, task, queue, args)
        self.properties = config.redis

    def run(self):
        """
        Trigger failover command.
        """
        self.mlock.lock_cluster(sorted(self.args['hosts']))
        failover_hosts = self.args.get("failover_hosts", [])
        host_group = build_host_group(self.properties, self.args['hosts'])
        self._health_host_group(host_group)

        host = get_first_key(self.args['hosts'])
        pillar = None

        if len(failover_hosts) != 0:
            host = failover_hosts[0]
            pillar = {'ensure_not_master': host}

        self.deploy_api.wait(
            [
                self._run_operation_host(
                    host,
                    'failover',
                    self.args['hosts'][host]['environment'],
                    pillar=pillar,
                    rollback=Change.noop_rollback,
                )
            ]
        )
        self.mlock.unlock_cluster()
