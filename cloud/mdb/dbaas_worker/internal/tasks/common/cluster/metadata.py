"""
Metadata state executor
"""

from ...common.deploy import BaseDeployExecutor


class ClusterMetadataExecutor(BaseDeployExecutor):
    """
    Metadata state executor
    """

    def run(self):
        self.mlock.lock_cluster(sorted(self.args['hosts']))
        self.deploy_api.wait(
            [
                self._run_operation_host(
                    host,
                    'metadata',
                    self.args['hosts'][host]['environment'],
                )
                for host in self.hosts_to_run()
            ]
        )
        self.mlock.unlock_cluster()

    def hosts_to_run(self):
        """
        Returns list of host on which quick operation should be executed
        """
        return self.args['hosts']
