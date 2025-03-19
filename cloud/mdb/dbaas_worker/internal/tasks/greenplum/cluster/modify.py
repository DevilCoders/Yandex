"""
Greenplum Modify cluster executor
"""

from ...common.cluster.modify import ClusterModifyExecutor
from ...utils import register_executor, get_managed_hostname
from ..utils import host_groups
from ..utils import GreenplumExecutorTrait
from ....providers.metadb_disk import MetadbDisks
from ....providers.juggler import JugglerApi

RECOVER_GREENPLUM_SEGMENT_HOSTS_FORCE_SLS = 'components.greenplum.recover_segments_force'

DOWN_GREENPLUM_SEGMENT_HOST_SLS = 'components.greenplum.segment_do_down'

RESTART_GREENPLUM_SEGMENT_HOST_SLS = 'components.greenplum.restart_segments_force'

REBALANCE_GREENPLUM_SEGMENTS_SLS = 'components.greenplum.rebalance_segments'

RESTART_GREENPLUM_SEGMENTS_SLS = 'components.greenplum.restart'

CHECK_GREENPLUM_MASTER_HOST_SLS = 'components.greenplum.check_master_befor_resize'

SWITCH_GREENPLUM_MASTER_HOST_DOWN_SLS = 'components.greenplum.switch_master_on_master'

SWITCH_GREENPLUM_MASTER_HOST_UP_SLS = 'components.greenplum.recover_master_force'

SWITCH_GREENPLUM_MASTER_HOST_PG_HBA_SLS = 'components.greenplum.update_pg_hba'

SWITCH_GREENPLUM_MASTER_HOST_ADD_STANDBY_SLS = 'components.greenplum.add_standby_greenplum_nocheck'


@register_executor('greenplum_cluster_modify')
class GreenplumClusterModify(ClusterModifyExecutor, GreenplumExecutorTrait):
    """
    Modify greenplum cluster
    """

    def __init__(self, config, task, queue, args):
        super().__init__(config, task, queue, args)
        self.metadb_disk = MetadbDisks(config, task, queue)
        self.juggler = JugglerApi(config, task, queue)

    def run(self):
        self.mlock.lock_cluster(sorted(self.args['hosts']))

        hosts = self.args['hosts']
        for host, opts in self.args['hosts'].items():
            if opts['vtype'] == 'compute' and opts['disk_type_id'] == 'network-ssd-nonreplicated':
                local_id, mount_point = self.metadb_disk.get_disks_info(host)
                opts['pg_num'] = local_id
                opts['disk_mount_point'] = mount_point

        master_host_group, segment_host_group = host_groups(
            hosts, self.config.greenplum_master, self.config.greenplum_segment
        )

        # check health before start
        self._gp_cluster_health(master_host_group, segment_host_group)

        for host, opts in master_host_group.hosts.items():
            managed_hostname = get_managed_hostname(host, opts)
            self.juggler.downtime_exists(managed_hostname, duration=self.task['timeout'])

        hosts = self.args['hosts']
        master_host_group, segment_host_group = host_groups(
            hosts, self.config.greenplum_master, self.config.greenplum_segment
        )

        needRebalance = False

        needRestart = self.args.get('restart', False)

        for host, opts in segment_host_group.hosts.items():
            opts['host_recreate_force'] = True
            opts['pre_restart_run'] = False

            self.logger.info('processing greenplum segment host ' + host)

            def before_recreate():
                self.logger.info('check greenplum segment host ' + host)

                for hostM in master_host_group.hosts:
                    recover_segment_host = [
                        self._run_sls_host(
                            hostM,
                            DOWN_GREENPLUM_SEGMENT_HOST_SLS,
                            pillar={'segment_fqdn': host},
                            environment=self.args['hosts'][hostM]['environment'],
                            title=host + "_br",
                        ),
                    ]
                    self.deploy_api.wait(recover_segment_host)

            def after_recreate():
                nonlocal needRebalance
                nonlocal needRestart

                self.logger.info('recover greenplum segment host ' + host)

                for hostM in master_host_group.hosts:
                    master_pillar = {}
                    master_pillar['segment_fqdn'] = host
                    recover_segment_host = [
                        self._run_sls_host(
                            hostM,
                            RECOVER_GREENPLUM_SEGMENT_HOSTS_FORCE_SLS,
                            pillar=master_pillar,
                            environment=self.args['hosts'][hostM]['environment'],
                            title=host + "_ar",
                        ),
                    ]
                    self.deploy_api.wait(recover_segment_host)

                self.logger.info('restart greenplum segment host ' + host)
                for hostM in master_host_group.hosts:
                    master_pillar = {}
                    master_pillar['segment_fqdn'] = host
                    recover_segment_host = [
                        self._run_sls_host(
                            hostM,
                            RESTART_GREENPLUM_SEGMENT_HOST_SLS,
                            pillar=master_pillar,
                            environment=self.args['hosts'][hostM]['environment'],
                            title=host + "_ar",
                        ),
                    ]
                    self.deploy_api.wait(recover_segment_host)

                self.logger.info('rebalance greenplum segment host ' + host)
                for hostM in master_host_group.hosts:
                    master_pillar = {}
                    recover_segment_host = [
                        self._run_sls_host(
                            hostM,
                            REBALANCE_GREENPLUM_SEGMENTS_SLS,
                            pillar=master_pillar,
                            environment=self.args['hosts'][hostM]['environment'],
                            title=host + "_ar",
                        ),
                    ]
                    self.deploy_api.wait(recover_segment_host)

                needRebalance = True
                needRestart = True

            self._modify_hosts(
                segment_host_group.properties,
                hosts={host: opts},
                do_before_recreate=before_recreate,
                do_after_recreate=after_recreate,
                restart=False,
            )

        for host, opts in master_host_group.hosts.items():
            opts['host_recreate_force'] = True
            opts['pre_restart_run'] = False

            self.logger.info('processing greenplum master host ' + host)

            alt_host = ''
            for ahost, aopts in master_host_group.hosts.items():
                if ahost != host:
                    alt_host = ahost

            def before_recreate():
                self.logger.info('check greenplum master host ' + host + ' -> ' + alt_host)

                check_master_host = [
                    self._run_sls_host(
                        alt_host,
                        CHECK_GREENPLUM_MASTER_HOST_SLS,
                        pillar={},
                        environment=self.args['hosts'][host]['environment'],
                        title=alt_host + "_check",
                    ),
                ]
                self.deploy_api.wait(check_master_host)

                self.logger.info('stop greenplum master host ' + host + ' -> ' + alt_host)
                stop_master_host = [
                    self._run_sls_host(
                        host,
                        SWITCH_GREENPLUM_MASTER_HOST_DOWN_SLS,
                        pillar={},
                        environment=self.args['hosts'][host]['environment'],
                        title=host + "_stop",
                    ),
                ]
                self.deploy_api.wait(stop_master_host)

                self.logger.info('up greenplum master host ' + host + ' -> ' + alt_host)
                up_master_host = [
                    self._run_sls_host(
                        alt_host,
                        SWITCH_GREENPLUM_MASTER_HOST_UP_SLS,
                        pillar={},
                        environment=self.args['hosts'][host]['environment'],
                        title=alt_host + "_up",
                    ),
                ]
                self.deploy_api.wait(up_master_host)

                add_standby_master_host = [
                    self._run_sls_host(
                        alt_host,
                        SWITCH_GREENPLUM_MASTER_HOST_ADD_STANDBY_SLS,
                        pillar={'standby_master_fqdn': host, 'check_if_exists': True},
                        environment=self.args['hosts'][host]['environment'],
                        title=alt_host + "_add_standby_befor",
                    ),
                ]
                self.deploy_api.wait(add_standby_master_host)

                self.logger.info('ph_hba for premodify greenplum master host ' + host + ' -> ' + alt_host)
                pg_hba_master_host = [
                    self._run_sls_host(
                        alt_host,
                        SWITCH_GREENPLUM_MASTER_HOST_PG_HBA_SLS,
                        pillar={},
                        environment=self.args['hosts'][host]['environment'],
                        title=alt_host + "_pg_hba",
                    ),
                ]
                self.deploy_api.wait(pg_hba_master_host)

            def after_recreate():
                nonlocal needRestart

                self.logger.info('add standby greenplum master host ' + host + ' -> ' + alt_host)
                add_standby_master_host = [
                    self._run_sls_host(
                        alt_host,
                        SWITCH_GREENPLUM_MASTER_HOST_ADD_STANDBY_SLS,
                        pillar={'standby_master_fqdn': host, 'check_if_exists': True},
                        environment=self.args['hosts'][host]['environment'],
                        title=alt_host + "_add_standby_after",
                    ),
                ]
                self.deploy_api.wait(add_standby_master_host)

                self.logger.info('ph_hba for aftermodify greenplum master host ' + host + ' -> ' + alt_host)
                pg_hba_upd_master_host = [
                    self._run_sls_host(
                        alt_host,
                        SWITCH_GREENPLUM_MASTER_HOST_PG_HBA_SLS,
                        pillar={},
                        environment=self.args['hosts'][host]['environment'],
                        title=alt_host + "_pg_hba_upd",
                    ),
                ]
                self.deploy_api.wait(pg_hba_upd_master_host)

                needRestart = True

            default_pillar = {'sync-passwords': self.args.get('sync-passwords', False)}
            self.logger.info('processing greenplum master host ' + host)
            self._modify_hosts(
                master_host_group.properties,
                hosts={host: opts},
                do_before_recreate=before_recreate,
                do_after_recreate=after_recreate,
                restart=False,
                default_pillar=default_pillar,
            )

        if needRestart:
            self.logger.info('greenplum cluster restart')
            for hostM in master_host_group.hosts:
                master_pillar = {}
                recover_segment_host = [
                    self._run_sls_host(
                        hostM,
                        RESTART_GREENPLUM_SEGMENTS_SLS,
                        pillar=master_pillar,
                        environment=self.args['hosts'][hostM]['environment'],
                    ),
                ]
                self.deploy_api.wait(recover_segment_host)

        if needRebalance:
            self.logger.info('greenplum cluster rebalance')
            for hostM in master_host_group.hosts:
                master_pillar = {}
                recover_segment_host = [
                    self._run_sls_host(
                        hostM,
                        REBALANCE_GREENPLUM_SEGMENTS_SLS,
                        pillar=master_pillar,
                        environment=self.args['hosts'][hostM]['environment'],
                        title=host + "_end",
                    ),
                ]
                self.deploy_api.wait(recover_segment_host)

        for host, opts in master_host_group.hosts.items():
            managed_hostname = get_managed_hostname(host, opts)
            self.juggler.downtime_absent(managed_hostname)

        self.mlock.unlock_cluster()
