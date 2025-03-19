"""
Greenplum host create executor.
"""
from copy import deepcopy

from ...common.host.create import HostCreateExecutor
from ...utils import build_host_group
from ...utils import register_executor
from ..utils import classify_host_map


@register_executor('greenplum_host_create')
class GreenplumHostCreate(HostCreateExecutor):
    """
    Create Greenplum host in dbm or compute.
    """

    def __init__(self, config, task, queue, args):
        super().__init__(config, task, queue, args)
        self.properties = deepcopy(self.config.greenplum_segment)
        _, gpdb_segment_hosts = classify_host_map(self.args['hosts'])
        self.properties.conductor_group_id = next(iter(gpdb_segment_hosts.values()))['subcid']

    def _create_host(self):
        """
        Reusable create host (without lock)
        """
        hosts_to_create = self.args['hosts_create_segments']
        existing_hosts = {}
        create_hosts = {}
        for host, attrs in self.args['hosts'].items():
            if host in hosts_to_create:
                create_hosts[host] = attrs
            else:
                existing_hosts[host] = attrs

        existing = build_host_group(self.properties, existing_hosts)
        create = build_host_group(self.properties, create_hosts)
        pillar = self.properties.create_host_pillar.copy()
        pillar.update(
            {'sync-timeout': self.deploy_api.get_seconds_to_deadline() // self.deploy_api.default_max_attempts}
        )
        for opts in create.hosts.values():
            opts['deploy'] = {
                'pillar': pillar,
                'title': 'created',
            }
        self._create_host_secrets(create)
        self._create_host_group(create, revertable=True)
        self._issue_tls(create)
        self._run_operation_host_group(existing, 'metadata')
        self._highstate_host_group(create)
        self._create_public_records(create)
        self._enable_monitoring(create)
