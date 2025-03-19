"""
Salt deploy module
"""
# pylint: disable=too-few-public-methods
from ..bootstrap import Bootstrap
from .base import BaseDeploy
from .commands import HighstateCommand, PsqlCommand


@Bootstrap.deploy_handler('metadb')
class MetaDBDeploy(BaseDeploy):
    """
    Deploy class for metadb
    """
    def __init__(self, config, global_config):
        super().__init__(config, global_config)
        self.master = self.hosts[0]
        self.replicas = self.hosts[1:]
        self.master_commands = [
            HighstateCommand(),
            PsqlCommand('INSERT INTO dbaas.config_host_access_ids VALUES ('
                        '\'{access_id}\', \'{secret}\', true, default);'.format(
                            access_id=self.global_config.api_access_id,
                            secret=self.global_config.api_encrypted_access_secret)),
        ]
        self.replica_commands = [
            HighstateCommand(pillar={'pg-master': self.master}, count=1),
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
