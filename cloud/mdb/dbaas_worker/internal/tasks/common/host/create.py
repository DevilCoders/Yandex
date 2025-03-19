"""
Common host create executor.
"""

from ...utils import build_host_group
from ..create import BaseCreateExecutor


# pylint: disable=too-many-instance-attributes
class HostCreateExecutor(BaseCreateExecutor):
    """
    Base class for host create executors
    """

    def __init__(self, config, task, queue, args):
        super().__init__(config, task, queue, args)
        self.properties = None

    def _create_host(self):
        """
        Reusable create host (without lock)
        """
        existing = build_host_group(
            self.properties, {x: y for x, y in self.args['hosts'].items() if x != self.args['host']}
        )
        create = build_host_group(
            self.properties, {x: y for x, y in self.args['hosts'].items() if x == self.args['host']}
        )
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

    def run(self):
        """
        Default behavior for Create Host task
        """
        self.acquire_lock()
        self._create_host()
        self.release_lock()
