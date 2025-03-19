"""
MongoDB stepdown given hosts
"""
from ....providers.common import Change
from ...common.deploy import BaseDeployExecutor
from ...utils import build_host_group, group_host_list_by_shards, register_executor, to_host_map
from ..utils import MONGOD_HOST_TYPE, classify_host_map


@register_executor('mongodb_cluster_stepdown_hosts')
class MongodbStepdownHosts(BaseDeployExecutor):
    """
    Stepdown one or more hosts
    """

    def run(self):
        self.mlock.lock_cluster(sorted(self.args['hosts']))
        hosts = classify_host_map({host: self.args['hosts'][host] for host in self.args['host_names']})

        host_groups = {
            host_type: build_host_group(getattr(self.config, host_type), hosts[host_type]) for host_type in hosts
        }

        if MONGOD_HOST_TYPE not in host_groups or len(host_groups[MONGOD_HOST_TYPE].hosts.keys()) <= 0:
            # Do not try to stepdown MongoS or MongoCFG hosts
            self.mlock.unlock_cluster()
            return

        hgroup = host_groups[MONGOD_HOST_TYPE]

        # Get all MongoD hosts in cluster, so we'll be able to check their health by shard
        mongod_hosts = build_host_group(
            getattr(self.config, MONGOD_HOST_TYPE),
            classify_host_map(self.args['hosts'])[MONGOD_HOST_TYPE],
        )
        for fqdn, opts in mongod_hosts.hosts.items():
            opts['fqdn'] = fqdn
        all_shards = group_host_list_by_shards(mongod_hosts.hosts.values())

        for fqdn, opts in hgroup.hosts.items():
            opts.update(
                {
                    'deploy': {
                        'pillar': {
                            'stepdown-id': self.args.get('stepdown_id', None),
                            'host-names': self.args['host_names'],
                        },
                    },
                    'fqdn': fqdn,
                }
            )

        shards = group_host_list_by_shards(hgroup.hosts.values())

        # Now we need to separate hosts to groups, one host from shard in group
        while shards:
            # hdict - dict{shard: host_for_stepdown}
            hdict = {}
            # other_hosts - other hosts in shards, so we can check their health
            other_hosts = []
            for shard in list(shards.keys()):
                hdict[shard] = shards[shard].pop()
                if not shards[shard]:
                    shards.pop(shard, None)
                for host in all_shards[shard]:
                    if host['fqdn'] != hdict[shard]['fqdn']:
                        other_hosts.append(host)

            # Check health of hosts which are not going to stepdown
            self._health_host_group(build_host_group(hgroup.properties, to_host_map(other_hosts)))

            # Run stepdown for given hosts (in parallel)
            self._run_operation_host_group(
                host_group=build_host_group(hgroup.properties, to_host_map(hdict.values())),
                operation='stepdown-host',
                rollback=Change.noop_rollback,
            )
        self.mlock.unlock_cluster()
