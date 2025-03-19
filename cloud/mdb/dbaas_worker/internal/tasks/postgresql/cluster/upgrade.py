"""
PostgreSQL Upgrade cluster executor.
"""
import os

from ....exceptions import UserExposedException
from ....providers.common import Change
from ....providers.compute import ComputeApi
from ....providers.deploy import DEPLOY_VERSION_V2, DeployErrorMaxAttempts
from ....providers.host_health import HostHealth
from ....providers.metadb_version import MetadbVersions
from ....providers.pgsync import PgSync, pgsync_cluster_prefix
from ....providers.zookeeper import Zookeeper
from ....utils import get_first_key, get_paths
from ...common.cluster.modify import ClusterModifyExecutor
from ...utils import build_host_group, register_executor


class UpgradeFailed(UserExposedException):
    """
    pg_upgrade --check returns error or other rollback reason
    """


class CriticalUpgradeFailed(RuntimeError):
    """
    error while upgrading, cannot rollback
    """


def get_major_version_num(version):
    """
    major_num in pillar
    Examples: 9.6 -> 906; 10 -> 1000
    """
    if '.' in version:
        major, minor = version.split('.')
    else:
        major = version
        minor = 0
    return str(100 * int(major) + int(minor))


class PostgreSQLClusterUpgrade(ClusterModifyExecutor):
    """
    Abstract class for upgrading PostgreSQL.
    """

    VERSION_FROM = ""
    VERSION_TO = ""

    def __init__(self, config, task, queue, args):
        if not self.VERSION_FROM or not self.VERSION_TO:
            raise NotImplementedError
        super().__init__(config, task, queue, args)
        self.pgsync = PgSync(self.config, self.task, self.queue)
        self.host_health = HostHealth(self.config, self.task, self.queue)
        self.compute = ComputeApi(self.config, self.task, self.queue)
        self.zookeeper = Zookeeper(self.config, self.task, self.queue)
        self.properties = self.config.postgresql
        self.versions = MetadbVersions(self.config, self.task, self.queue)
        self.addresses = {}

    def _execute_host(self, host, *commands, timeout=60, rollback=None):
        if timeout is None:
            timeout = self._get_exec_timeout()
        deploy_version = self.deploy_api.get_deploy_version_from_minion(host)
        if deploy_version == DEPLOY_VERSION_V2:
            method = {
                'commands': [
                    {
                        'type': 'cmd.retcode',
                        'arguments': [x, 'python_shell=True', 'timeout={timeout}'.format(timeout=timeout)],
                        'timeout': timeout,
                    }
                    for x in commands
                ],
                'fqdns': [host],
                'parallel': 1,
                'stopOnErrorCount': 1,
                'timeout': timeout * len(commands),
            }
        else:
            raise Exception('invalid deploy version: {version}'.format(version=deploy_version))

        return self.deploy_api.run(
            host,
            method=method,
            deploy_version=deploy_version,
            deploy_title=','.join(commands),
            rollback=rollback,
        )

    def _install_tmp_ssh_keys(self, master_fqdn, host_group):
        state_type = 'mdb_postgresql.generate_ssh_keys'
        shipment_id = self._run_sls_host(
            master_fqdn,
            '',
            pillar=None,
            environment=self.args['hosts'][master_fqdn]['environment'],
            state_type=state_type,
            title='generate_ssh_keys',
            rollback=lambda task, safe_revision: self.deploy_api.wait(
                [
                    self._execute_host(
                        master_fqdn,
                        "rm -f /root/.ssh/id_rsa",
                    )
                ]
            ),
        )
        self.deploy_api.wait([shipment_id])

        public_key = self.deploy_api.get_result_for_shipment(shipment_id, master_fqdn, state_type)
        paths = get_paths(host_group.properties.host_os)

        self.deploy_api.wait(
            [
                self._execute_host(
                    host,
                    "echo '{key}' >> {path}".format(key=public_key, path=paths.AUTHORIZED_KEYS2),
                    rollback=lambda task, safe_revision: self.deploy_api.wait(
                        [
                            self._execute_host(
                                host,
                                "sed -i '/{master_fqdn}/d' {path}".format(
                                    master_fqdn=master_fqdn, path=paths.AUTHORIZED_KEYS2
                                ),
                            )
                        ]
                    ),
                )
                for host in host_group.hosts
            ]
        )

    def _set_setup_addresses(self, hosts):
        for host, opts in hosts.items():
            if opts['vtype'] == 'compute':
                self.addresses[host] = self.compute.get_instance_setup_address(host)

    def _install_pkgs(self):
        self.deploy_api.wait(
            [
                self._run_sls_host(
                    host,
                    'components.postgresql_cluster.operations.install-packages',
                    pillar=None,
                    environment=self.args['hosts'][host]['environment'],
                    rollback=Change.noop_rollback,
                )
                for host in self.args['hosts']
            ]
        )

    def _cleanup(self, host_group, master_fqdn):
        self._run_operation_host_group(
            host_group,
            'cleanup-after-upgrade',
            pillar={'version_from': self.VERSION_FROM, 'master_fqdn': master_fqdn},
        )

    def _upgrade_cluster(self, fqdn):
        def rollback(master_fqdn, safe_revision):
            host_group = build_host_group(self.properties, self.args['hosts'])
            for fqdn, opts in host_group.hosts.items():
                opts['deploy'] = {'pillar': {'rev': safe_revision}}
            self._highstate_host_group(host_group)

        self.deploy_api.wait(
            [
                self._run_sls_host(
                    fqdn,
                    'components.postgres.configs.pg_upgrade_cluster',
                    pillar=None,
                    environment=self.args['hosts'][fqdn]['environment'],
                    title='update pg_upgrade_cluster.py script',
                    rollback=Change.noop_rollback,
                ),
            ]
        )

        state_type = 'mdb_postgresql.upgrade'
        shipment_id = self._run_sls_host(
            fqdn,
            '{version_to}'.format(version_to=self.VERSION_TO),
            pillar={'cluster_node_addrs': self.addresses},
            environment=self.args['hosts'][fqdn]['environment'],
            state_type=state_type,
            title='upgrade',
            timeout=self.deploy_api.get_seconds_to_deadline(),
            rollback=lambda task, safe_revision: rollback(fqdn, safe_revision),
        )
        try:
            self.deploy_api.wait([shipment_id], max_attempts=1)
        except DeployErrorMaxAttempts:
            self.context_cache.add_change(Change('upgrade_failed', True))
            raise CriticalUpgradeFailed('Unexpected error occurred')

        result = self.deploy_api.get_result_for_shipment(shipment_id, fqdn, state_type)
        if not result:
            self.logger.error("upgrade state not found for shipment %s", shipment_id)
            self.context_cache.add_change(Change('upgrade_failed', True))
            raise UpgradeFailed('Unknown error, rolled back')

        if not result.get('is_upgraded'):
            self.logger.error(result)
            if result.get('is_rolled_back'):
                raise UpgradeFailed(result.get('user_exposed_error', 'Unknown error, rolled back'))
            else:
                self.context_cache.add_change(Change('upgrade_failed', True))
                raise CriticalUpgradeFailed('Unknown error, rollback is not possible')
        else:
            self.context_cache.add_change(Change('upgrade_completed', True))

    def run(self):
        self._set_setup_addresses(self.args['hosts'])
        self.mlock.lock_cluster(sorted(self.args['hosts']))
        self.update_major_version('postgres')
        host_group = build_host_group(self.properties, self.args['hosts'])
        self._health_host_group(host_group, '-pre-upgrade')

        if len(self.args['hosts']) == 1:
            master = get_first_key(self.args['hosts'])
        else:
            master = self.pgsync.get_master(self.args['zk_hosts'], pgsync_cluster_prefix(self.task['cid']))
            replicas_host_group = {host: opts for host, opts in self.args['hosts'].items() if host != master}
            self._install_tmp_ssh_keys(master, build_host_group(self.properties, replicas_host_group))

        self._install_pkgs()
        self.pgsync.start_maintenance(self.args['zk_hosts'], pgsync_cluster_prefix(self.task['cid']))

        self._upgrade_cluster(master)
        self.zookeeper.absent(self.args['zk_hosts'], os.path.join(pgsync_cluster_prefix(self.task['cid']), 'timeline'))

        if self.VERSION_TO == '12':
            for fqdn, opts in host_group.hosts.items():
                if fqdn != master:
                    opts['deploy'] = {'pillar': {'service-restart': True}}

        self._highstate_host_group(host_group)
        self.pgsync.stop_maintenance(self.args['zk_hosts'], pgsync_cluster_prefix(self.task['cid']))
        self._health_host_group(host_group)
        self._cleanup(host_group, master)
        self.mlock.unlock_cluster()


@register_executor('postgresql_cluster_upgrade_11_1c')
@register_executor('postgresql_cluster_upgrade_11')
class PostgreSQLClusterUpgrade11(PostgreSQLClusterUpgrade):
    """
    Upgrade postgresql cluster to version 11
    """

    VERSION_FROM = '10'
    VERSION_TO = '11'


@register_executor('postgresql_cluster_upgrade_12')
class PostgreSQLClusterUpgrade12(PostgreSQLClusterUpgrade):
    """
    Upgrade postgresql cluster to version 12
    """

    VERSION_FROM = '11'
    VERSION_TO = '12'


@register_executor('postgresql_cluster_upgrade_13')
class PostgreSQLClusterUpgrade13(PostgreSQLClusterUpgrade):
    """
    Upgrade postgresql cluster to version 13
    """

    VERSION_FROM = '12'
    VERSION_TO = '13'


@register_executor('postgresql_cluster_upgrade_14')
class PostgreSQLClusterUpgrade14(PostgreSQLClusterUpgrade):
    """
    Upgrade postgresql cluster to version 14
    """

    VERSION_FROM = '13'
    VERSION_TO = '14'
