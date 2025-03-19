"""
Base deploy module
"""
# pylint: disable=too-few-public-methods
import operator

from ..bootstrap import Bootstrap
from .commands import (CreateSecretsCommand, HighstateCommand, SetHostnameCommand, SSHCommand)


class BaseDeploy:
    """
    Base class to deploy host group
    """

    # pylint: disable=anomalous-backslash-in-string
    def __init__(self, config, global_config):
        self.config = config
        self.global_config = global_config
        self.commands = [
            SetHostnameCommand(),
            CreateSecretsCommand(),
            SSHCommand(commands=[
                'echo "::1  {host} localhost" > /etc/hosts',
                "sed -i 's/01234567890123456789/{token}/'"
                " /etc/yandex/selfdns-client/default.conf",
                "sed -i /etc/yandex/selfdns-client/plugins/default -e "
                r"'/echo.*IPV4_ADDRESS/s/echo \"/echo >\/dev\/null \"/'",
                'selfdns-client --terminal --debug',
                'rm -f /tmp/.grains_conductor.cache',
                'salt-call saltutil.sync_all',
                'echo ",+," | sfdisk --force -N1 /dev/vda || /bin/true',
                'partprobe /dev/vda',
                'resize2fs /dev/vda1',
                'service bind9 restart',
            ],
                       token=self.global_config.selfdns_token),
        ]
        self.hosts = list(map(operator.itemgetter('fqdn'), self.config['hosts']))

    def _my_salt(self):
        return self.global_config.internal_salt

    def deploy(self, compute):
        """
        Main method to start deploy
        """
        for host in self.hosts:
            for cmd in self.commands:
                cmd.execute(host, compute)


@Bootstrap.deploy_handler('common')
class CommonDeploy(BaseDeploy):
    """
    Common deploy class. It prepares hosts and runs highstate.
    """
    def __init__(self, config, global_config):
        super().__init__(config, global_config)
        self.commands.append(HighstateCommand())


@Bootstrap.deploy_handler('zk')
class ZKDeploy(CommonDeploy):
    """
    ZooKeeper deploy class. Restarts ZK on all hosts.
    """
    def deploy(self, compute):
        """
        Main method to make all magic
        """
        def _deploy(compute):
            for host in self.hosts:
                for cmd in self.commands:
                    cmd.execute(host, compute)

        _deploy(compute)
        self.commands = [
            SSHCommand(commands=[
                'service zookeeper restart',
            ]),
        ]
        _deploy(compute)
