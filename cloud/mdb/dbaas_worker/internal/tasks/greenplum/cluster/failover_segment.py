"""
Greenplum cluster segment failover executor
"""

from ...common.deploy import BaseDeployExecutor
from ...utils import register_executor
from ..utils import host_groups, GreenplumExecutorTrait
from ....providers.metadb_host import MetadbHost

FAILOVER_GREENPLUM_SEGMENT_HOSTS_FORCE_SLS = 'components.greenplum.failover_segment'


@register_executor('greenplum_cluster_start_segment_failover')
class GreenplumStartSegmentFailover(BaseDeployExecutor, GreenplumExecutorTrait):
    """
    Start failover on segment GreenplumSQL subcluster
    """

    def __init__(self, config, task, queue, args):
        super().__init__(config, task, queue, args)
        self.metadb_host = MetadbHost(config, task, queue)

    def run(self):
        """
        Trigger failover command.
        """
        self.mlock.lock_cluster(sorted(self.args['hosts']))

        hosts = self.args['hosts']
        master_host_group, segment_host_group = host_groups(
            hosts, self.config.greenplum_master, self.config.greenplum_segment
        )

        # check health before start
        self._gp_cluster_health(master_host_group, segment_host_group)
        target_maint_vtype_id = self.args.get('target_maint_vtype_id')

        target_host = self.metadb_host.get_host_info_by_vtype_d(vtype_id=target_maint_vtype_id)

        failover_segment_host = [
            self._run_sls_host(
                target_host,
                FAILOVER_GREENPLUM_SEGMENT_HOSTS_FORCE_SLS,
                pillar={},
                environment=self.args['hosts'][target_host]['environment'],
                title=target_host + " failover",
            ),
        ]
        self.deploy_api.wait(failover_segment_host)

        self.mlock.unlock_cluster()
