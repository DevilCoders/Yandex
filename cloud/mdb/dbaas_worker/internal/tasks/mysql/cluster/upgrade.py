"""
MySQL Upgrade cluster executor.
"""

from ....providers.host_health import HostHealth
from ....providers.metadb_version import MetadbVersions
from ....providers.mysync import MySync
from ....providers.zookeeper import Zookeeper
from ...common.cluster.modify import ClusterModifyExecutor
from ....utils import get_first_key
from ...utils import build_host_group, register_executor
from .upgrade_checker_mixin import UpgradeCheckerMixin
from typing import List, Tuple


def merge_dict(src1, src2):
    """
    high-level dictionary merge helper
    """
    if src1 is None:
        src1 = {}
    if src2 is None:
        src2 = {}
    res = src1.copy()
    for key, val in src2.items():
        res[key] = val
    return res


class MySQLClusterUpgrade(ClusterModifyExecutor, UpgradeCheckerMixin):
    """
    Abstract class for upgrading MySQL.
    """

    VERSION_FROM = ""
    VERSION_TO = ""

    REPL_LAG_CHECK = 'mysql_replication_lag'

    def __init__(self, config, task, queue, args):
        if not self.VERSION_FROM or not self.VERSION_TO:
            raise NotImplementedError
        super().__init__(config, task, queue, args)
        self.mysync = MySync(self.config, self.task, self.queue)
        self.host_health = HostHealth(self.config, self.task, self.queue)
        self.zookeeper = Zookeeper(self.config, self.task, self.queue)
        self.versions = MetadbVersions(self.config, self.task, self.queue)
        self.properties = self.config.mysql

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
        cid, zk_hosts = self.task['cid'], self.args['zk_hosts']

        self.mysync.start_maintenance(zk_hosts, cid)

        # check health before starting upgrade
        host_group = build_host_group(self.properties, self.args['hosts'])
        host_group.properties.juggler_checks.append(self.REPL_LAG_CHECK)
        self._health_host_group(host_group, timeout=300)

        # run upgrade_checker:
        replicas, master = self.get_replicas_and_master()
        check_on_host = replicas[0] if len(replicas) != 0 else master  # checker may get locks, so prefer replicas
        self.upgrade_checker_must_succeed(self.task['cid'], check_on_host)

        # run pre_upgrade operation to hack configs
        self._run_operation_host_group(
            host_group, 'service', title='pre-upgrade-config-fix', pillar={'pre_upgrade': True}
        )

        # upgrade replicas
        for host in replicas:
            opts = host_group.hosts[host]
            self.deploy_api.wait(
                [
                    self.deploy_api.run(
                        host,
                        pillar=merge_dict(
                            opts.get('deploy', {}).get('pillar'), {'upgrade': True, 'service-restart': True}
                        ),
                        deploy_title=opts.get('deploy', {}).get('title'),
                    ),
                ]
            )

        # check health before upgrading master
        self._health_host_group(host_group, timeout=self.deploy_api.get_seconds_to_deadline())

        ha_replicas = self.mysync.get_ha_nodes(self.args['zk_hosts'], self.task['cid'])
        if len(ha_replicas) > 1:
            # switch the master to the upgraded replica
            self.mysync.stop_maintenance(zk_hosts, cid)
            self.mysync.ensure_replica(zk_hosts, cid, master)
            self.mysync.start_maintenance(zk_hosts, cid)

            # update privileges on new master (5.7 => 8 specific ?)
            new_master = self.mysync.get_master(self.args['zk_hosts'], self.task['cid'])
            opts = host_group.hosts[new_master]
            self.deploy_api.wait(
                [
                    self.deploy_api.run(
                        new_master,
                        pillar=opts.get('deploy', {}).get('pillar'),
                        deploy_title='update-privileges',
                    ),
                ]
            )

        # upgrade (old or single) master
        opts = host_group.hosts[master]
        self.deploy_api.wait(
            [
                self.deploy_api.run(
                    master,
                    pillar=merge_dict(opts.get('deploy', {}).get('pillar'), {'upgrade': True, 'service-restart': True}),
                    deploy_title=opts.get('deploy', {}).get('title'),
                ),
            ]
        )

        # check health before applying post_upgrade:
        self._health_host_group(host_group, timeout=self.deploy_api.get_seconds_to_deadline())

        # run post_upgrade operation to fix configs
        self._run_operation_host_group(
            host_group, 'service', title='post-upgrade-config-fix', pillar={'post_upgrade': True}
        )

        self.mysync.stop_maintenance(zk_hosts, cid)
        self.mlock.unlock_cluster()


@register_executor('mysql_cluster_upgrade_80')
class MySQLClusterUpgrade80(MySQLClusterUpgrade):
    """
    Upgrade mysql cluster to version 8.0
    """

    VERSION_FROM = '5.7'
    VERSION_TO = '8.0'
