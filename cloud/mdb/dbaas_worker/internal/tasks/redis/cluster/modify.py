"""
Redis Modify cluster executor
"""
from ...common.cluster.modify import ClusterModifyExecutor
from ...utils import register_executor, build_host_group
from ..utils import get_one_by_one_order, up_timeouts


@register_executor('redis_cluster_modify')
class RedisClusterModify(ClusterModifyExecutor):
    """
    Modify Redis cluster
    """

    def check_downscale_available(self, hosts):
        if not self.args.get('memory_downscaled'):
            return

        def rollback(rollback_hosts, safe_revision):
            host_group = build_host_group(self.config.redis, rollback_hosts)
            for opts in host_group.hosts.values():
                opts['deploy'] = {'pillar': {'rev': safe_revision}}
            self._run_operation_host_group(host_group, 'service', title='reverting maxmemory downscale')

        state_type = 'mdb_redis.check_downscale_available'
        for host, opts in hosts.items():
            shipment_id = self._run_sls_host(
                host,
                '',
                pillar=None,
                environment=hosts[host]['environment'],
                state_type=state_type,
                title='check-downscale-available-{host}'.format(host=host),
                rollback=lambda task, safe_revision: rollback({host: opts}, safe_revision),
            )
            self.deploy_api.wait([shipment_id])

    def run(self):
        """
        Run modify on redis hosts
        """
        hosts = self.args['hosts']
        masters = self.args.get('masters')
        self.mlock.lock_cluster(sorted(hosts))
        self.update_major_version('redis')
        up_timeouts(hosts)
        if self.args.get('restart'):
            self.check_downscale_available(hosts)
            self._modify_hosts(self.config.redis, order=get_one_by_one_order(hosts, masters))
        else:
            default_pillar = {'sentinel-restart': True} if self.args.get('restart_only_sentinel') else None
            self._modify_hosts(self.config.redis, default_pillar=default_pillar)
        self.mlock.unlock_cluster()
