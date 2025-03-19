"""
MongoDB Delete host executor
"""

from ....utils import filter_host_map, get_first_value, get_host_role
from ...common.host.delete import HostDeleteExecutor
from ...utils import build_host_group, build_host_group_from_list, register_executor
from ..utils import ORDERED_HOST_TYPES, ROLE_HOST_TYPE, classify_host_map


@register_executor('mongodb_host_delete')
class MongoDBHostDelete(HostDeleteExecutor):
    """
    Delete mongodb host
    """

    def run(self):
        lock_hosts = list(self.args['hosts'])
        lock_hosts.append(self.args['host']['fqdn'])
        self.mlock.lock_cluster(sorted(lock_hosts))
        hosts = classify_host_map(self.args['hosts'])
        host_groups = {
            host_type: build_host_group(getattr(self.config, host_type), hosts[host_type]) for host_type in hosts
        }

        for host_type in ORDERED_HOST_TYPES:
            if host_type not in host_groups:
                continue
            hgroup = host_groups[host_type]
            self._run_operation_host_group(hgroup, 'metadata')

        for host_type in ORDERED_HOST_TYPES:
            if host_type not in host_groups:
                continue
            hgroup = host_groups[host_type]
            self._health_host_group(hgroup)

        delete_host = self.args['host']
        hosts = filter_host_map(
            self.args['hosts'],
            subcid=delete_host['subcid'],
            shard_id=delete_host['shard_id'],
        )
        delete_host_role = self.args.get('hosts_role', None) or get_host_role(get_first_value(hosts), ROLE_HOST_TYPE)
        delete_host_group = build_host_group_from_list(
            getattr(self.config, ROLE_HOST_TYPE[delete_host_role]),
            [self.args['host']],
        )
        self._delete_host_group_full(delete_host_group)

        if len(hosts) == 1:
            # Last host of this subcluster, need to delete group from conductor as well
            self.conductor.group_absent(
                getattr(delete_host_group.properties, 'conductor_group_id', self.task['cid']),
            )

        self.mlock.unlock_cluster()
