"""
ClickHouse maintenance cluster executor
"""
from ....providers.clickhouse.cloud_storage import CloudStorage
from ...common.cluster.maintenance import ClusterMaintenanceExecutor
from ...utils import build_host_group, get_managed_hostname, register_executor, split_hosts_for_modify_by_shard_batches
from ..utils import ch_build_host_groups, update_zero_copy_required


@register_executor('clickhouse_cluster_maintenance')
class ClickHouseClusterMaintenance(ClusterMaintenanceExecutor):
    """
    Maintenance ClickHouse cluster
    """

    def __init__(self, config, task, queue, args):
        super().__init__(config, task, queue, args)
        self.cloud_storage = CloudStorage(self.config, self.task, self.queue, self.args)

    def run(self):
        self.mlock.lock_cluster(sorted(self.args['hosts']))
        if self._is_offline_maintenance():
            # Sometimes too old hosts can't start without highstate(i.e. expired cert), so omit checks here.
            if 'disable_health_check' not in self.args:
                self.args['disable_health_check'] = True
            self.args['update_tls'] = True

        ch_group, zk_group = ch_build_host_groups(
            self.args['hosts'],
            self.config,
            ignored_hosts=self.args.get('ignore_hosts', []),
        )
        self._start_offline_maintenance(zk_group)
        self._start_offline_maintenance(ch_group)

        self._health_host_group(ch_group)
        self._health_host_group(zk_group)

        if self.args.get('deploy_zk_tls', False):
            self._deploy_zk_tls(ch_group, zk_group)
        elif update_zero_copy_required(self.args, ch_group, zk_group, self.cloud_storage):
            self._major_upgrade_with_zero_copy_convertion(ch_group, zk_group)
        else:
            self._general_maintenance(ch_group, zk_group)

        self._stop_offline_maintenance(ch_group)
        self._stop_offline_maintenance(zk_group)
        self.mlock.unlock_cluster()

    def _general_maintenance(self, ch_group, zk_group):
        # Issue new TLS certificates if required
        update_tls = self.args.get('update_tls', False)
        if update_tls:
            force_tls_certs = self.args.get('force_tls_certs', False)
            self._issue_tls(ch_group, force_tls_certs=force_tls_certs)
            self._issue_tls(zk_group, force_tls_certs=force_tls_certs)

        # Maintain ZooKeeper hosts
        self._highstate_host_group(zk_group, title="maintenance")

        if self.args.get('restart_zk', False) or update_tls:
            self._health_host_group(zk_group, '-pre-restart')
            self._run_operation_host_group(
                zk_group,
                'service',
                title='post-maintenance-restart',
                pillar={'service-restart': True},
                order=zk_group.hosts.keys(),
            )

        # Maintain ClickHouse hosts
        restart_ch_hosts = self.args.get('restart', False) or update_tls
        if restart_ch_hosts:
            for opts in ch_group.hosts.values():
                opts['deploy'] = {'pillar': {'service-restart': True}}

        for i_hosts in split_hosts_for_modify_by_shard_batches(
            ch_group.hosts, fast_mode=self.args.get('fast_mode', False)
        ):
            i_group = build_host_group(ch_group.properties, i_hosts)
            if restart_ch_hosts:
                for host, opts in i_group.hosts.items():
                    managed_hostname = get_managed_hostname(host, opts)
                    self.juggler.downtime_exists(managed_hostname, duration=self.task['timeout'])
            self._highstate_host_group(i_group, title='maintenance')
            self._health_host_group(i_group, '-post-maintenance')
            if restart_ch_hosts:
                for host, opts in i_group.hosts.items():
                    managed_hostname = get_managed_hostname(host, opts)
                    self.juggler.downtime_absent(managed_hostname)

    def _deploy_zk_tls(self, ch_group, zk_group):
        # Issue new TLS certificates if required
        self._issue_tls(ch_group, force_tls_certs=True)
        self._issue_tls(zk_group, force_tls_certs=True)

        # ZK stage 1 - allow TLS in config, use plain text for CH and quorum, ignore ACL, turn on port unification
        for opts in zk_group.hosts.values():
            opts['deploy'] = {'pillar': {'service-restart': True, 'tls-stage': 1}}

        for host in zk_group.hosts:
            i_group = build_host_group(zk_group.properties, {host: zk_group.hosts[host]})
            self._highstate_host_group(i_group, title='tls-stage1-maintenance')
            self._health_host_group(i_group, '-tls-stage1-maintenance')

        # CH stage - use TSL for ZK, send ACL creds
        for opts in ch_group.hosts.values():
            opts['deploy'] = {'pillar': {'service-restart': True}}

        for i_hosts in split_hosts_for_modify_by_shard_batches(
            ch_group.hosts, fast_mode=self.args.get('fast_mode', False)
        ):
            i_group = build_host_group(ch_group.properties, i_hosts)
            for host, opts in i_group.hosts.items():
                managed_hostname = get_managed_hostname(host, opts)
                self.juggler.downtime_exists(managed_hostname, duration=self.task['timeout'])
            self._highstate_host_group(i_group, title='maintenance')
            self._health_host_group(i_group, '-tls-maintenance')
            for host, opts in i_group.hosts.items():
                managed_hostname = get_managed_hostname(host, opts)
                self.juggler.downtime_absent(managed_hostname)

        # ZK stage 2 - use ACL, use TLS quorum
        for opts in zk_group.hosts.values():
            opts['deploy'] = {'pillar': {'service-restart': True, 'tls-stage': 2}}

        for host in zk_group.hosts:
            i_group = build_host_group(zk_group.properties, {host: zk_group.hosts[host]})
            self._highstate_host_group(i_group, title='tls-stage2-maintenance')
            self._health_host_group(i_group, '-tls-stage2-maintenance')

        # ZK stage 3 - turn off port unification
        for opts in zk_group.hosts.values():
            opts['deploy'] = {'pillar': {'service-restart': True, 'tls-stage': 3}}

        for host in zk_group.hosts:
            i_group = build_host_group(zk_group.properties, {host: zk_group.hosts[host]})
            self._highstate_host_group(i_group, title='tls-stage3-maintenance')
            self._health_host_group(i_group, '-tls-stage3-maintenance')

        self._health_host_group(ch_group, '-tls-post-maintenance')

    def _major_upgrade_with_zero_copy_convertion(self, ch_group, zk_group):
        # Issue new TLS certificates if required
        if self.args.get('update_tls', False):
            force_tls_certs = self.args.get('force_tls_certs', False)
            self._issue_tls(ch_group, force_tls_certs=force_tls_certs)
            self._issue_tls(zk_group, force_tls_certs=force_tls_certs)

        # Maintain ZooKeeper hosts
        self._highstate_host_group(zk_group, title="maintenance")

        if self.args.get('restart_zk', False):
            self._health_host_group(zk_group, '-pre-restart')
            self._run_operation_host_group(
                zk_group,
                'service',
                title='post-maintenance-restart',
                pillar={'service-restart': True},
                order=zk_group.hosts.keys(),
            )

        # Maintain ClickHouse hosts in compatibility mode
        for opts in ch_group.hosts.values():
            opts['deploy'] = {'pillar': {'service-restart': True, 'convert_zero_copy_schema': 'convert'}}

        for i_hosts in split_hosts_for_modify_by_shard_batches(
            ch_group.hosts, fast_mode=self.args.get('fast_mode', False)
        ):
            i_group = build_host_group(ch_group.properties, i_hosts)
            for host, opts in i_group.hosts.items():
                managed_hostname = get_managed_hostname(host, opts)
                self.juggler.downtime_exists(managed_hostname, duration=self.task['timeout'])
            self._highstate_host_group(i_group, title='update-zc-maintenance')
            for host, opts in i_group.hosts.items():
                managed_hostname = get_managed_hostname(host, opts)
                self.juggler.downtime_absent(managed_hostname)

        # Maintain ClickHouse hosts in ordinar mode
        for opts in ch_group.hosts.values():
            opts['deploy'] = {'pillar': {'service-restart': True, 'convert_zero_copy_schema': 'cleanup'}}

        for i_hosts in split_hosts_for_modify_by_shard_batches(
            ch_group.hosts, fast_mode=self.args.get('fast_mode', False)
        ):
            i_group = build_host_group(ch_group.properties, i_hosts)
            for host, opts in i_group.hosts.items():
                managed_hostname = get_managed_hostname(host, opts)
                self.juggler.downtime_exists(managed_hostname, duration=self.task['timeout'])
            self._highstate_host_group(i_group, title='maintenance')
            self._health_host_group(i_group, '-post-maintenance')
            for host, opts in i_group.hosts.items():
                managed_hostname = get_managed_hostname(host, opts)
                self.juggler.downtime_absent(managed_hostname)
