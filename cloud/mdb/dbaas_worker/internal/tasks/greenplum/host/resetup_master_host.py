"""
Greenplum Create host for recrate master executor
"""

from ...common.host.create import HostCreateExecutor
from .....internal.exceptions import ExposedException
from ..utils import host_groups
from ....providers.metadb_disk import MetadbDisks
from ......internal.python.compute.disks import DiskType


RECOVER_GREENPLUM_MASTER_HOSTS_FORCE_SLS = 'components.greenplum.recover_master_force'
RECOVER_GREENPLUM_MASTER_HOSTS_ADD_SLS = 'components.greenplum.add_standby_greenplum_nocheck'


class GreenplumMasterHostReSetup(HostCreateExecutor):
    """
    ReSetup greenplum host in dbm or compute
    """

    def __init__(self, config, task, queue, args):
        super().__init__(config, task, queue, args)
        self.properties = self.config.greenplum_master
        self.metadb_disk = MetadbDisks(config, task, queue)

        self.properties.conductor_group_id = self.args['subcid']

    def _create_host(self):
        """
        Create host (without lock) for GP resetup host
        """
        fqdn = self.args['host']
        hosts = self.args['hosts']

        for host, opts in self.args['hosts'].items():
            if opts['vtype'] == 'compute' and opts['disk_type_id'] == DiskType.network_ssd_nonreplicated.value:
                local_id, mount_point = self.metadb_disk.get_disks_info(host)
                opts['pg_num'] = local_id
                opts['disk_mount_point'] = mount_point

        master_host_group, segment_host_group = host_groups(
            hosts, self.config.greenplum_master, self.config.greenplum_segment
        )

        if len(master_host_group.hosts) < 2:
            raise NotImplementedError('Resetup for cluster with 1 master host is not avaliable')

        super()._create_host()

        master_host_fqdn = ""
        for host in master_host_group.hosts:
            if host == fqdn:
                continue
            master_pillar = {}
            recover_second_master_host = [
                self._run_sls_host(
                    host,
                    RECOVER_GREENPLUM_MASTER_HOSTS_FORCE_SLS,
                    pillar=master_pillar,
                    environment=self.args['hosts'][host]['environment'],
                ),
            ]
            self.deploy_api.wait(recover_second_master_host)
            master_host_fqdn = host
            break

        if master_host_fqdn == "":
            raise ExposedException('The alternative master host is not running')

        master_pillar = {}
        master_pillar['standby_master_fqdn'] = fqdn
        master_pillar['kill_replica'] = True

        recover_add_master_host = [
            self._run_sls_host(
                master_host_fqdn,
                RECOVER_GREENPLUM_MASTER_HOSTS_ADD_SLS,
                pillar=master_pillar,
                environment=self.args['hosts'][master_host_fqdn]['environment'],
            ),
        ]
        self.deploy_api.wait(recover_add_master_host)

        master_host_group.hosts[master_host_fqdn]['deploy'] = {
            'pillar': {
                'gpdb_master': True,
                'master_fqdn': master_host_fqdn,
            },
        }
        self._highstate_host_group(master_host_group)
        self._highstate_host_group(segment_host_group)
