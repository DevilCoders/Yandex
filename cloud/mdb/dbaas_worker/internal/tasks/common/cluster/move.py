"""
Cluster move executor
"""

from ...common.deploy import BaseDeployExecutor


class ClusterMoveExecutor(BaseDeployExecutor):
    """
    Cluster move executor
    """

    def run(self):
        self.mlock.lock_cluster(sorted(self.args['hosts']))
        deploys = []
        for host, opts in self.args['hosts'].items():
            sls = ['components.common.dbaas', 'components.dbaas-billing.billing']
            deploys.append(self._run_sls_host(host, *sls, pillar={}, environment=opts['environment']))

        self.deploy_api.wait(deploys)
        self.mlock.unlock_cluster()
