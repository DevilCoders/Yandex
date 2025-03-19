"""
SSH module
"""
import os
import signal
import subprocess  # noqa

import paramiko
from paramiko.ssh_exception import SSHException

from dbaas_common import retry, tracing

from ..utils import HostOS, get_paths
from .common import BaseProvider


class SSHClient(BaseProvider):
    """
    SSH client provider
    """

    def __init__(self, config, task, queue):
        super().__init__(config, task, queue)
        self.agent = None
        self.environ = {}

    # pylint: disable=no-self-use
    def setup_agent(self, key_path):
        """
        Run ssh-agent and add key to it
        """
        if not self.agent:
            pid = None
            ret = subprocess.check_output('/usr/bin/ssh-agent')  # noqa
            for line in ret.decode('utf-8').splitlines():
                if 'export ' in line:
                    key, value = line.split(';')[0].split('=')
                    if key == 'SSH_AGENT_PID':
                        pid = int(value)
                    self.environ.update({key: value})
            subprocess.check_call(['/usr/bin/ssh-add', key_path], env=self.environ)  # noqa
            self.agent = pid
            for key, value in self.environ.items():
                os.environ[key] = value

    def __del__(self):
        """
        Kill ssh-agent
        """
        if self.agent:
            os.kill(self.agent, signal.SIGTERM)
        for key in self.environ:
            if key in os.environ:
                del os.environ[key]

    def get_pub_key(self):
        """
        Return temporary ssh key
        """
        return self.config.ssh.public_key[:]

    @tracing.trace('SSH Connect')
    def get_conn(self, connect_address, host_os: str = HostOS.LINUX.value, use_agent: bool = False):
        """
        Get ssh and sftp conn to address
        """
        tracing.set_tag('ssh.target.fqdn', connect_address)
        tracing.set_tag('ssh.target.os', host_os)

        if use_agent:
            self.setup_agent(self.config.ssh.private_key)
        ssh = paramiko.SSHClient()
        ssh_pkey = paramiko.ecdsakey.ECDSAKey(filename=self.config.ssh.private_key)
        ssh.load_system_host_keys()
        ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
        username = 'Administrator' if host_os == HostOS.WINDOWS.value else 'root'
        ssh.connect(
            connect_address, username=username, pkey=ssh_pkey, timeout=5, banner_timeout=30, look_for_keys=False
        )
        session = ssh.get_transport().open_session()  # type: ignore
        paramiko.agent.AgentRequestHandler(session)
        session.get_pty()
        session.exec_command('tty')  # noqa
        return ssh

    # pylint: disable=no-self-use
    def get_sftp_conn(self, ssh):
        """
        Get sftp connection from opened ssh client
        """
        return paramiko.SFTPClient.from_transport(ssh.get_transport())

    @retry.on_exception(SSHException, factor=5, max_wait=20, max_tries=3)
    @tracing.trace('SSH Exec Command')
    def exec_command(self, fqdn, ssh, cmd, env=None, allow_fail=False, interruptable=False):
        """
        ssh.exec_command wrapper
        """
        tracing.set_tag('ssh.target.fqdn', fqdn)

        self.logger.info('Executing %s on %s', cmd, fqdn)
        _, stdout, stderr = ssh.exec_command(cmd, environment=env)  # noqa
        if interruptable:
            with self.interruptable:
                status = stdout.channel.recv_exit_status()
        else:
            status = stdout.channel.recv_exit_status()
        if status != 0 and not allow_fail:
            message = (
                f'Running {cmd} on {fqdn} failed with {status}.\n' f'Stdout: {stdout.read()}\nStderr: {stderr.read()}'
            )
            self.logger.error(message)
            raise SSHException(message)
        self.logger.info('Command %s finished with status %s', cmd, status)
        return status == 0, stdout.read(), stderr.read()

    @tracing.trace('SSH Cleanup Key')
    def cleanup_key(self, connect_address, host_os: str = HostOS.LINUX.value):
        """
        Cleanup worker key from root's authorized_keys2
        """
        tracing.set_tag('ssh.target.fqdn', connect_address)
        tracing.set_tag('ssh.target.os', host_os)

        paths = get_paths(host_os)
        with self.get_conn(connect_address, host_os) as ssh:
            sftp = self.get_sftp_conn(ssh)
            with sftp.open(paths.AUTHORIZED_KEYS2, 'r+') as fobj:
                lines = fobj.readlines()  # pylint: disable=no-member
                buffer = ''
                for line in lines:
                    if self.config.ssh.key_user not in line:
                        buffer += line
                fobj.seek(0)  # pylint: disable=no-member
                fobj.truncate(0)  # pylint: disable=no-member
                fobj.write(buffer)  # pylint: disable=no-member
