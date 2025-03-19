"""
Salt deploy module
"""
# pylint: disable=too-few-public-methods
from ..bootstrap import Bootstrap
from .base import BaseDeploy
from .commands import HighstateCommand


@Bootstrap.deploy_handler('mdb-health')
class MDBHealthDeploy(BaseDeploy):
    """
    Deploy class for mdb-health with redis
    """
    def __init__(self, config, global_config):
        super().__init__(config, global_config)
        self.master = self.hosts[0]
        self.replicas = self.hosts[1:]
        self.master_commands = [HighstateCommand()]
        self.replica_commands = [
            HighstateCommand(pillar={'redis-master': self.master}, count=1),
        ]

    def deploy(self, compute):
        """
        We need to use different deploys on master and replicas
        """
        super().deploy(compute)
        for cmd in self.master_commands:
            cmd.execute(self.master, compute)
        for host in self.replicas:
            for cmd in self.replica_commands:
                cmd.execute(host, compute)
