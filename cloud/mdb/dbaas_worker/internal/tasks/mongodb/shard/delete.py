"""
MongoDB shard delete executor.
"""

from ....utils import get_first_value
from ...common.shard.delete import ShardDeleteExecutor
from ...utils import build_host_group, register_executor
from ..utils import MONGOD_HOST_TYPE, MONGOS_HOST_TYPE, MONGOINFRA_HOST_TYPE, ORDERED_HOST_TYPES, classify_host_map


@register_executor('mongodb_shard_delete')
class MongodbShardDeleteExecuter(ShardDeleteExecutor):
    """
    Delete MongoDB shard.
    """

    def __init__(self, config, task, queue, args):
        super().__init__(config, task, queue, args)
        self.properties = getattr(self.config, MONGOD_HOST_TYPE)

    def run(self):
        lock_hosts = list(self.args['hosts'])
        lock_hosts.extend([host['fqdn'] for host in self.args['shard_hosts']])
        self.mlock.lock_cluster(sorted(lock_hosts))
        hosts_classified = classify_host_map(self.args['hosts'])
        host_groups = {
            host_type: build_host_group(getattr(self.config, host_type), hosts_classified[host_type])
            for host_type in hosts_classified
        }
        for hgroup in host_groups:
            self._health_host_group(host_groups[hgroup])

        used_mongos_host_type = MONGOS_HOST_TYPE
        if MONGOS_HOST_TYPE not in host_groups:
            used_mongos_host_type = MONGOINFRA_HOST_TYPE
        first_mongos = get_first_value(host_groups[used_mongos_host_type].hosts)
        first_mongos['deploy'] = {
            'pillar': {
                'mongodb-delete-shard-name': self.args['shard_name'],
            },
            'title': 'delete_shard',
        }

        for host_type in ORDERED_HOST_TYPES:
            if host_type not in host_groups or not host_groups[host_type].hosts:
                # Do not process subclusters if there is no hosts of such type
                continue
            hgroup = host_groups[host_type]
            self._run_operation_host_group(hgroup, 'metadata')

        self._delete_hosts()
        self.mlock.unlock_cluster()
