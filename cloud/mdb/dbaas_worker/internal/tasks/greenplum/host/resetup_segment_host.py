"""
Greenplum Create host for recrate segment executor
"""

from ...common.host.create import HostCreateExecutor
from ..utils import host_groups
from ...utils import get_managed_hostname
from ....providers.metadb_disk import MetadbDisks
from ......internal.python.compute.disks import DiskType


RECOVER_GREENPLUM_SEGMENT_HOSTS_FORCE_SLS = 'components.greenplum.recover_segments_force'
RESTART_GREENPLUM_SEGMENT_HOST_SLS = 'components.greenplum.restart_segments_force'
BEFORE_RESETUP_TASK_SLS = "components.greenplum.tasks.resetup.before_resetup"
AFTER_RESETUP_TASK_SLS = "components.greenplum.tasks.resetup.after_resetup"


class GreenplumSegmentHostReSetup(HostCreateExecutor):
    """
    ReSetup greenplum host in dbm or compute
    """

    def __init__(self, config, task, queue, args):
        super().__init__(config, task, queue, args)
        self.properties = self.config.greenplum_segment
        self.metadb_disk = MetadbDisks(config, task, queue)

        self.properties.conductor_group_id = self.args['subcid']

    def before_create_host(self):
        master_host_group, segment_host_group = host_groups(
            self.args["hosts"], self.config.greenplum_master, self.config.greenplum_segment
        )
        master_pillar = {"segment_fqdn": self.args["host"]}
        self.logger.info('running before resetup tasks on host {}'.format(self.args["host"]))

        for host in master_host_group.hosts:
            stop_segments_on_host = [
                self._run_sls_host(
                    host,
                    BEFORE_RESETUP_TASK_SLS,
                    pillar=master_pillar,
                    environment=self.args['hosts'][host]['environment'],
                ),
            ]
            self.deploy_api.wait(stop_segments_on_host)

    def after_create_host(self):
        master_host_group, segment_host_group = host_groups(
            self.args["hosts"], self.config.greenplum_master, self.config.greenplum_segment
        )
        master_pillar = {"segment_fqdn": self.args["host"]}
        self.logger.info('running after resetup tasks on host {}'.format(self.args["host"]))

        for host in master_host_group.hosts:
            stop_segments_on_host = [
                self._run_sls_host(
                    host,
                    AFTER_RESETUP_TASK_SLS,
                    pillar=master_pillar,
                    environment=self.args['hosts'][host]['environment'],
                ),
            ]
            self.deploy_api.wait(stop_segments_on_host)

    def _create_host(self):
        """
        Create host (without lock) for GP resetup host
        """
        for host, opts in self.args['hosts'].items():
            if opts['vtype'] == 'compute' and opts['disk_type_id'] == DiskType.network_ssd_nonreplicated.value:
                local_id, mount_point = self.metadb_disk.get_disks_info(host)
                opts['pg_num'] = local_id
                opts['disk_mount_point'] = mount_point

        hosts = self.args['hosts']
        master_host_group, segment_host_group = host_groups(
            hosts, self.config.greenplum_master, self.config.greenplum_segment
        )

        try:
            for host in master_host_group.hosts:
                opts = master_host_group.hosts[host]
                managed_hostname = get_managed_hostname(host, opts)
                self.juggler.downtime_exists(managed_hostname, 600)

            super()._create_host()

            for host in master_host_group.hosts:
                master_pillar = {}
                master_pillar['segment_fqdn'] = self.args['host']
                recover_segment_host = [
                    self._run_sls_host(
                        host,
                        RECOVER_GREENPLUM_SEGMENT_HOSTS_FORCE_SLS,
                        pillar=master_pillar,
                        environment=self.args['hosts'][host]['environment'],
                    ),
                ]
                self.deploy_api.wait(recover_segment_host)

            self._highstate_host_group(segment_host_group)
            self._highstate_host_group(master_host_group)

            for host in master_host_group.hosts:
                master_pillar = {}
                master_pillar['segment_fqdn'] = self.args['host']
                recover_segment_host = [
                    self._run_sls_host(
                        host,
                        RESTART_GREENPLUM_SEGMENT_HOST_SLS,
                        pillar=master_pillar,
                        environment=self.args['hosts'][host]['environment'],
                    ),
                ]
                self.deploy_api.wait(recover_segment_host)

        except Exception:
            for host in master_host_group.hosts:
                opts = master_host_group.hosts[host]
                managed_hostname = get_managed_hostname(host, opts)
                self.juggler.downtime_absent(managed_hostname)
            raise
