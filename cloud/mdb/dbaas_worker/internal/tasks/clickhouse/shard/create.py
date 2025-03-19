"""
ClickHouse shard create executor.
"""

from ....utils import get_first_key, split_host_map
from ...common.shard.create import ShardCreateExecutor
from ...utils import build_host_group, register_executor
from ..utils import classify_host_map, create_schema_backup, delete_ch_backup


@register_executor('clickhouse_shard_create')
class ClickhouseShardCreateExecuter(ShardCreateExecutor):
    """
    Create ClickHouse shard.
    """

    def run(self):
        self.mlock.lock_cluster(sorted(self.args['hosts']))
        ch_hosts, zk_hosts = classify_host_map(self.args['hosts'], ignored_hosts=self.args.get('ignore_hosts', []))

        shard_ch_hosts, rest_ch_hosts = split_host_map(ch_hosts, self.args['shard_id'])
        ch_subcid = next(iter(ch_hosts.values()))['subcid']

        zookeeper = build_host_group(self.config.zookeeper, zk_hosts)
        existing = build_host_group(self.config.clickhouse, rest_ch_hosts)

        create = build_host_group(self.config.clickhouse, shard_ch_hosts)
        create.properties.conductor_group_id = ch_subcid

        for host in create.hosts:
            create.hosts[host]['service_account_id'] = self.args.get('service_account_id')
            create.hosts[host]['deploy'] = {'pillar': {'restart-check': False}}

        first_creating = get_first_key(create.hosts)
        create.hosts[first_creating]['deploy']['pillar'].update({'do-backup': True})

        backup_id = None
        if self.args.get('resetup_from_replica', False):
            backup_host = next(iter(rest_ch_hosts))
            backup_shard = ch_hosts[backup_host].get('shard_name')
            backup_id = create_schema_backup(self.deploy_api, backup_host)
            for host in create.hosts:
                create.hosts[host]['deploy']['pillar'].update(
                    {
                        'replica_schema_backup': backup_id,
                        'schema_backup_shard': backup_shard,
                    }
                )

        self._create_host_secrets(create)
        self._create_host_group(create, revertable=True)
        self._issue_tls(create)
        self._run_operation_host_group(zookeeper, 'metadata')
        self._run_operation_host_group(existing, 'metadata')
        self._highstate_host_group(create)
        self._create_public_records(create)
        self._enable_monitoring(create)

        if self.args.get('resetup_from_replica', False):
            delete_ch_backup(self.deploy_api, next(iter(rest_ch_hosts)), backup_id)

        self.mlock.unlock_cluster()
