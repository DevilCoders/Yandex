"""
Greenplum Create cluster executor
"""
from ....providers.gp_ssh_key import GreenplumSshKeyProvider
from ...common.cluster.create import ClusterCreateExecutor
from ...utils import register_executor
from ..utils import host_groups
from ....providers.metadb_disk import MetadbDisks
from ....providers.metadb_host import MetadbHost

CREATE = 'greenplum_cluster_create'
RESTORE = 'greenplum_cluster_restore'

CONFIG_POSTGRESQL_SLS = 'components.greenplum.configs.postgresql-conf'
RESTART_GREENPLUM_SLS = 'components.greenplum.restart_greenplum_cluster'


@register_executor(CREATE)
@register_executor(RESTORE)
class GreenplumClusterCreate(ClusterCreateExecutor):
    """
    Create greenplum cluster in dbm and/or compute
    """

    def __init__(self, config, task, queue, args):
        super().__init__(config, task, queue, args)
        self.gp_ssh_key = GreenplumSshKeyProvider(config, task, queue)
        self.metadb_disk = MetadbDisks(config, task, queue)
        self.metadb_host = MetadbHost(config, task, queue)

    def run(self):
        hosts = self.args['hosts']

        self.mlock.lock_cluster(sorted(hosts))
        task_type = self.task['task_type']

        for host, opts in hosts.items():
            if opts['vtype'] == 'compute':
                if opts['disk_type_id'] == 'network-ssd-nonreplicated':
                    local_id, mount_point = self.metadb_disk.get_disks_info(host)
                    opts['pg_num'] = local_id
                    opts['disk_mount_point'] = mount_point
                pg_local_id = self.metadb_host.get_host_info(host)['local_id']
                if pg_local_id is not None:
                    opts['pg_local_id'] = pg_local_id

        master_host_group, segment_host_group = host_groups(
            hosts, self.config.greenplum_master, self.config.greenplum_segment
        )

        master = sorted(master_host_group.hosts)[0]
        standby_master = None
        if len(master_host_group.hosts) > 1:
            standby_master = sorted(master_host_group.hosts)[1]
            master_standby_pillar = self.make_standby_pillar(task_type, master, self.args)
            master_host_group.hosts[standby_master]['deploy'] = {
                'pillar': master_standby_pillar,
                'title': 'standby_master',
            }

        master_pillar = self.make_master_pillar(task_type, master, standby_master, self.args)
        segment_pillar = self.make_segment_pillar(task_type, master, self.args)

        master_host_group.hosts[master]['deploy'] = {
            'pillar': master_pillar,
            'title': 'master',
        }

        for host in segment_host_group.hosts:
            segment_host_group.hosts[host]['deploy'] = {
                'pillar': segment_pillar,
                'title': 'segment',
            }

        groups = [master_host_group] + [segment_host_group]

        self.gp_ssh_key.exists(self.task['cid'])
        self._create(*groups)
        if task_type == RESTORE:
            # Need to run another highstate on segments to remove the restore keys and configs
            self._highstate_host_group(segment_host_group)
        else:
            self._run_operation_host_group(
                segment_host_group, 'service', title='postgresql-conf', pillar=segment_pillar
            )

        self._run_operation_host_group(
            master_host_group,
            'service',
            title='postgresql-conf',
            pillar=master_pillar,
        )

        self.restart_greenplum_after_apply_postgresqlconf(master, master_pillar)
        self.deploy_api.wait(
            [
                self._run_operation_host(
                    master,
                    'backup',
                    environment=self.args['hosts'][master]['environment'],
                    pillar=master_pillar,
                )
            ]
        )
        self.mlock.unlock_cluster()

    def restart_greenplum_after_apply_postgresqlconf(self, master, master_pillar):
        cluster_restart = [
            self._run_sls_host(
                master,
                RESTART_GREENPLUM_SLS,
                pillar=master_pillar,
                environment=self.args['hosts'][master]['environment'],
            ),
        ]
        self.deploy_api.wait(cluster_restart)

    def make_master_pillar(self, task_type: str, master: str, standby_master: str, args: dict) -> dict:

        pillar = {
            'gpdb_master': True,
            'master_fqdn': master,
            'no_sleep': True,
        }
        if standby_master is not None:
            pillar['standby_master_fqdn'] = standby_master
            pillar['standby_install'] = True
        if task_type == RESTORE:
            self.add_restore_pillar_data(pillar, args)
        return pillar

    def make_standby_pillar(self, task_type: str, master: str, args: dict) -> dict:

        pillar = {
            'gpdb_standby_master': True,
            'master_fqdn': master,
        }
        if task_type == RESTORE:
            self.add_restore_pillar_data(pillar, args)
        return pillar

    def make_segment_pillar(self, task_type: str, master: str, args: dict) -> dict:

        pillar = {
            'gpdb_segment': True,
            'master_fqdn': master,
        }
        if task_type == RESTORE:
            self.add_restore_pillar_data(pillar, args)
        return pillar

    def add_restore_pillar_data(self, pillar: dict, args: dict):
        pillar['target-pillar-id'] = args['target-pillar-id']
        pillar['restore-from'] = args['restore-from']
