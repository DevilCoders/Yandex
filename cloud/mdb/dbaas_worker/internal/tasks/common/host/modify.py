"""
Common host modify executor.
"""

from ..modify import BaseModifyExecutor


class HostModifyExecutor(BaseModifyExecutor):
    """
    Generic class for host modify executors
    """

    def _modify_host(self, host, pillar=None):
        """
        Now it just call deploy to host
        """
        self._change_host_public_ip(host)
        self.deploy_api.wait(
            [self._run_operation_host(host, 'service', self.args['hosts'][host]['environment'], pillar=pillar)]
        )
