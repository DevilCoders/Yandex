"""
MongoDB shard create executor.
"""

from ....utils import get_first_value, split_host_map
from ...common.shard.create import ShardCreateExecutor
from ...utils import build_host_group, register_executor
from ..utils import (
    MONGOD_HOST_TYPE,
    MONGOS_HOST_TYPE,
    MONGOINFRA_HOST_TYPE,
    ORDERED_HOST_TYPES,
    build_new_replset_hostgroup,
    classify_host_map,
)


@register_executor('mongodb_shard_create')
class MongodbShardCreateExecuter(ShardCreateExecutor):
    """
    Create MongoDB shard.
    """

    def run(self):
        self.mlock.lock_cluster(sorted(self.args['hosts']))
        shard_id = self.args['shard_id']
        new_hosts, old_hosts = split_host_map(self.args['hosts'], shard_id)

        old_hosts_classified = classify_host_map(old_hosts)
        old_host_groups = {
            host_type: build_host_group(getattr(self.config, host_type), old_hosts_classified[host_type])
            for host_type in old_hosts_classified
        }
        for hgroup in old_host_groups:
            self._health_host_group(old_host_groups[hgroup])

        new_host_group = build_new_replset_hostgroup(
            new_hosts, getattr(self.config, MONGOD_HOST_TYPE), self.task, self.args
        )
        self._create_host_secrets(new_host_group)
        self._create_host_group(new_host_group, revertable=True)
        self._issue_tls(new_host_group)
        self._highstate_host_group(new_host_group)
        self._create_public_records(new_host_group)

        used_mongos_host_type = MONGOS_HOST_TYPE
        if MONGOS_HOST_TYPE not in old_host_groups:
            used_mongos_host_type = MONGOINFRA_HOST_TYPE
        first_mongos = get_first_value(old_host_groups[used_mongos_host_type].hosts)
        first_mongos['deploy'] = {
            'pillar': {
                'mongodb-add-shards': [shard_id],
            },
            'title': 'add_shard',
        }
        for host_type in ORDERED_HOST_TYPES:
            if host_type not in old_host_groups or not old_host_groups[host_type].hosts:
                # Do not process subclusters if there is no hosts of such type
                continue
            hgroup = old_host_groups[host_type]
            self._run_operation_host_group(hgroup, 'metadata')

        self._enable_monitoring(new_host_group)
        self.mlock.unlock_cluster()
