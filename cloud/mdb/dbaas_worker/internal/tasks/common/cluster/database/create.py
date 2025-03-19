"""
Database create executor
"""

from ....common.deploy import BaseDeployExecutor


class DatabaseCreateExecutor(BaseDeployExecutor):
    """
    Create database executor
    """

    def _make_pillar(self, host):
        return {'target-database': self.args['target-database']}

    def run(self):
        self.mlock.lock_cluster(sorted(self.args['hosts']))
        self.deploy_api.wait(
            [
                self._run_operation_host(
                    host,
                    'database-create',
                    self.args['hosts'][host]['environment'],
                    pillar=self._make_pillar(host),
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
