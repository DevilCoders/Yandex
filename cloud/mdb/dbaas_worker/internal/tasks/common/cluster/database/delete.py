"""
Database delete executor
"""

from ....common.deploy import BaseDeployExecutor


class DatabaseDeleteExecutor(BaseDeployExecutor):
    """
    Delete database executor
    """

    def run(self):
        self.mlock.lock_cluster(sorted(self.args['hosts']))
        self.deploy_api.wait(
            [
                self._run_operation_host(
                    host,
                    'database-delete',
                    self.args['hosts'][host]['environment'],
                    pillar={'target-database': self.args['target-database']},
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
