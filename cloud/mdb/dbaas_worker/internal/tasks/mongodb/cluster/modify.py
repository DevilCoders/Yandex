"""
MongoDB Modify cluster executor
"""

from copy import deepcopy

from ....utils import get_first_value
from ...common.cluster.modify import ClusterModifyExecutor
from ...utils import register_executor
from ..utils import ORDERED_HOST_TYPES, classify_host_map


@register_executor('mongodb_cluster_modify')
class MongoDBClusterModify(ClusterModifyExecutor):
    """
    Modify mongodb cluster
    """

    def run(self):
        self.mlock.lock_cluster(sorted(self.args['hosts']))
        hosts = classify_host_map(self.args['hosts'])
        restart_services = self.args.get('restart_services', [])
        restart_common = self.args.get('restart')
        run_highstate = None
        if self.args.get('include-performance_diagnostics'):
            run_highstate = True

        for host_type in ORDERED_HOST_TYPES:
            if host_type not in hosts:
                continue
            properties = deepcopy(getattr(self.config, host_type))
            properties.conductor_group_id = get_first_value(hosts[host_type])['subcid']
            self._modify_hosts(
                properties,
                hosts=hosts[host_type],
                restart=host_type in restart_services or restart_common,
                run_highstate=run_highstate,
            )
        self.mlock.unlock_cluster()
