"""
Module with commands
"""
# pylint: disable=too-few-public-methods
import json
from abc import ABC, abstractmethod
from socket import error as SocketError

import backoff
from dbaas_worker.providers.salt_secret import SaltSecret
from paramiko.ssh_exception import NoValidConnectionsError, SSHException


class Command(ABC):
    """
    Abstract class for command
    """
    @abstractmethod
    def execute(self, host, compute):
        """
        Execute command
        """


class SSHCommand(Command):
    """
    Class to exec ssh command on host
    """
    def __init__(self, commands, **params):
        if isinstance(commands, str):
            self.templates = [commands]
        else:
            self.templates = commands
        self.commands = list()
        self.params = params

    def _prepare(self, host):
        """
        Prepare command templates to execute
        """
        self.commands.clear()
        for cmd in self.templates:
            self.commands.append(cmd.format(host=host, **self.params))

    @backoff.on_exception(backoff.expo, (NoValidConnectionsError, SSHException, SocketError),
                          factor=10,
                          max_value=60,
                          max_tries=100)
    def execute(self, host, compute):
        """
        Execute command on host
        """
        self._prepare(host)
        connect_address = compute.get_instance_setup_address(host)
        with compute.ssh.get_conn(connect_address) as ssh:
            if isinstance(self.commands, list):
                for command in self.commands:
                    compute.ssh.exec_command(host, ssh, command)
            else:
                compute.ssh.exec_command(host, ssh, self.commands)


class SetHostnameCommand(SSHCommand):
    """
    Set hostname via ssh
    """
    def __init__(self):
        super().__init__(commands='echo {host} > /etc/hostname')


class CreateSecretsCommand(Command):
    """
    Command to create secrets on host
    """
    def execute(self, host, compute):
        """
        Execute command on host
        """
        generator = SaltSecret(compute.config, compute.task, compute.queue)
        secrets = generator.generate(host)
        compute.create_secrets(host, secrets)


class HighstateCommand(SSHCommand):
    """
    Command to run salt highstate on host
    """
    def __init__(self, pillar=None, count=2):
        cmd = 'salt-call state.highstate queue=True'
        if pillar is not None:
            cmd += " pillar='{pillar}'"
        cmd += ' | grep Failed: | tail -n1 | grep -q 0'
        super().__init__(commands=[cmd] * count, pillar=json.dumps(pillar))


class PsqlCommand(SSHCommand):
    """
    Class to execute psql with query
    """
    def __init__(self, query, dbname='dbaas_metadb'):
        super().__init__(commands=[
            'sudo -u postgres psql {dbname} -c "{query}"'.format(query=query, dbname=dbname),
        ])
