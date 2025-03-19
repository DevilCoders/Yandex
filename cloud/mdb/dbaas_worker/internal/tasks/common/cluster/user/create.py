"""
User create executor
"""

from ....common.deploy import BaseDeployExecutor


class UserCreateExecutor(BaseDeployExecutor):
    """
    Create user executor
    """

    def run(self):
        self.mlock.lock_cluster(sorted(self.args['hosts']))
        self.deploy_api.wait(
            [
                self._run_operation_host(
                    host,
                    'user-create',
                    self.args['hosts'][host]['environment'],
                    pillar={'target-user': self.args['target-user']},
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
