"""
MySQL cluster maintenance executor
"""

from ....providers.metadb_version import MetadbVersions
from ....providers.mysync import MySync
from ...common.cluster.maintenance import ClusterMaintenanceExecutor
from ...utils import build_host_group, register_executor
from ....utils import get_first_key
from .upgrade_checker_mixin import UpgradeCheckerMixin
from typing import List, Tuple


@register_executor('mysql_cluster_maintenance')
class MySQLClusterMaintenance(ClusterMaintenanceExecutor, UpgradeCheckerMixin):
    """
    Perform Maintenance on MySQL cluster
    """

    REPL_LAG_CHECK = 'mysql_replication_lag'

    def __init__(self, config, task, queue, args):
        super().__init__(config, task, queue, args)
        self.mysync = MySync(self.config, self.task, self.queue)
        self.versions = MetadbVersions(self.config, self.task, self.queue)

    def get_replicas_and_master(self) -> Tuple[List[str], str]:
        """
        Dynamic order resolver with mysync
        """
        if len(self.args['hosts']) == 1:
            return [], get_first_key(self.args['hosts'])
        master = self.mysync.get_master(self.args['zk_hosts'], self.task['cid'])
        return [x for x in self.args['hosts'] if x != master], master

    def run(self):
        self.mlock.lock_cluster(sorted(self.args['hosts']))
        host_group = build_host_group(self.config.mysql, self.args['hosts'])
        self._start_offline_maintenance(host_group)

        cid, zk_hosts = self.task['cid'], self.args['zk_hosts']
        self.mysync.start_maintenance(zk_hosts, cid)

        replicas, master = self.get_replicas_and_master()

        if self.args.get('upgrade', False):
            # run upgrade_checker:
            check_on_host = replicas[0] if len(replicas) != 0 else master  # checker may get locks, so prefer replicas
            self.upgrade_checker_must_succeed(cid, check_on_host)

        if self.args.get('restart', False):
            host_group.properties.juggler_checks.append(self.REPL_LAG_CHECK)
            # check health before starting upgrade
            self._health_host_group(host_group, timeout=300)
            # maintain replicas
            for host in replicas:
                group = build_host_group(self.config.mysql, {host: self.args['hosts'][host]})
                self._run_maintenance_task(
                    group, get_order=lambda: None, disable_monitoring=self.args.get('restart', False)
                )
            ha_replicas = self.mysync.get_ha_nodes(self.args['zk_hosts'], self.task['cid'])
            # switch the master to the upgraded replica
            if len(ha_replicas) > 1:
                self.mysync.stop_maintenance(zk_hosts, cid)
                self.mysync.ensure_replica(zk_hosts, cid, master)
                self.mysync.start_maintenance(zk_hosts, cid)
            # maintain master
            group = build_host_group(self.config.mysql, {master: self.args['hosts'][master]})
            self._run_maintenance_task(
                group, get_order=lambda: None, disable_monitoring=self.args.get('restart', False)
            )
        else:
            self._run_maintenance_task(host_group, get_order=lambda: replicas + [master])

        self.mysync.stop_maintenance(zk_hosts, cid)
        self._stop_offline_maintenance(host_group)
        self.mlock.unlock_cluster()
