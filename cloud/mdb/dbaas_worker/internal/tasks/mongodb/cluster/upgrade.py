"""
MongoDB Upgrade cluster executor
"""

from ...common.cluster.modify import ClusterModifyExecutor
from ...utils import build_host_group, group_host_list_by_shards, register_executor, to_host_map
from ..utils import (
    MONGOCFG_HOST_TYPE,
    MONGOD_HOST_TYPE,
    MONGOS_HOST_TYPE,
    MONGOINFRA_HOST_TYPE,
    ORDERED_HOST_TYPES,
    classify_host_map,
)


@register_executor('mongodb_cluster_upgrade')
class MongoDBClusterUpgrade(ClusterModifyExecutor):
    """
    Upgrade mongodb cluster
    """

    def run(self):
        self.mlock.lock_cluster(sorted(self.args['hosts']))
        hosts = classify_host_map(self.args['hosts'])
        host_groups = {
            host_type: build_host_group(getattr(self.config, host_type), hosts[host_type]) for host_type in hosts
        }

        if any(
            (
                MONGOS_HOST_TYPE in host_groups and host_groups[MONGOS_HOST_TYPE].hosts,
                MONGOINFRA_HOST_TYPE in host_groups and host_groups[MONGOINFRA_HOST_TYPE].hosts,
            )
        ):
            # It's sharded MongoDB cluster
            # https://docs.mongodb.com/master/release-notes/4.2-upgrade-sharded-cluster/

            for htype in ORDERED_HOST_TYPES:
                if htype not in host_groups:
                    continue
                self._health_host_group(host_groups[htype], '-pre-balancer-stop')

            # 1. Disable the Balancer
            hlist = []
            for htype in [MONGOS_HOST_TYPE, MONGOINFRA_HOST_TYPE]:
                if htype not in host_groups:
                    continue
                hlist += host_groups[htype].hosts

            self.deploy_api.wait(
                [
                    self._run_operation_host(
                        host,
                        'balancer-stop',
                        self.args['hosts'][host]['environment'],
                        title='mongos-balancer-stop',
                    )
                    for host in hlist
                ]
            )

            # 2. Upgrade the config servers.
            hlist = []
            for htype in [MONGOCFG_HOST_TYPE, MONGOINFRA_HOST_TYPE]:
                if htype not in host_groups:
                    continue
                hlist += host_groups[htype].hosts

            self.deploy_api.wait(
                [
                    self.deploy_api.run(
                        host,
                        pillar={
                            'service-restart': True,
                            'service-for-restart': 'mongocfg',
                            'skip-mongos': True,
                        },
                        deploy_title='mongocfg-upgrade',
                    )
                    for host in hlist
                ]
            )

            for htype in [MONGOCFG_HOST_TYPE, MONGOINFRA_HOST_TYPE]:
                if htype not in host_groups:
                    continue
                hgroup = host_groups[htype]
                self._health_host_group(hgroup, '-post-mongocfg-upgrage')

            # 3. Upgrade the shards.
            for fqdn, opts in host_groups[MONGOD_HOST_TYPE].hosts.items():
                opts.update(
                    {
                        'deploy': {
                            'pillar': {
                                'service-restart': True,
                                'service-for-restart': 'mongod',
                            },
                        },
                        'fqdn': fqdn,
                    }
                )

            shards = group_host_list_by_shards(host_groups[MONGOD_HOST_TYPE].hosts.values())
            for shard_id, shard in shards.items():
                # Upgrade the shards one at a time.
                host_group = build_host_group(host_groups[MONGOD_HOST_TYPE].properties, to_host_map(shard))
                self._highstate_host_group(host_group, f'mongod-{shard_id}-upgrade')
                self._health_host_group(host_groups[MONGOD_HOST_TYPE], f'-post-shard-{shard_id}')

            # 4. Upgrade the mongos instances.
            hlist = []
            for htype in [MONGOS_HOST_TYPE, MONGOINFRA_HOST_TYPE]:
                if htype not in host_groups:
                    continue
                hlist += host_groups[htype].hosts

            self.deploy_api.wait(
                [
                    self.deploy_api.run(
                        host,
                        pillar={
                            'service-restart': True,
                            'service-for-restart': 'mongos',
                            'skip-mongocfg': True,
                        },
                        deploy_title='mongos-upgrade',
                    )
                    for host in hlist
                ]
            )

            for htype in [MONGOS_HOST_TYPE, MONGOINFRA_HOST_TYPE]:
                if htype not in host_groups:
                    continue
                hgroup = host_groups[htype]
                self._health_host_group(hgroup, '-post-mongos-upgrade')

            # 5. Re-enable the balancer.
            # hlist is already contain MongoS + MongoInfra
            self.deploy_api.wait(
                [
                    self._run_operation_host(
                        host,
                        'balancer-start',
                        self.args['hosts'][host]['environment'],
                        title='mongos-balancer-start',
                    )
                    for host in hlist
                ]
            )

        else:
            # Non-sharded MongoDB cluster
            host_group = build_host_group(self.config.mongod, self.args['hosts'])
            for opts in host_group.hosts.values():
                opts['deploy'] = {'pillar': {'service-restart': True}}
            self._highstate_host_group(host_group)
        self.mlock.unlock_cluster()
