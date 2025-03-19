"""
Simple ssh mock
"""

from io import StringIO

from .utils import handle_action


class ChannelMock:
    """
    ssh channel mock
    """

    def recv_exit_status(self):
        """
        always success
        """
        return 0


class StreamMock:
    """
    ssh stdout/stderr mock
    """

    def __init__(self):
        self.channel = ChannelMock()

    def read(self):
        """
        constant string
        """
        return 'output'.encode('utf-8')


def exec_command(state, cmd, environment=None):
    """
    ssh.exec_command mock
    """
    if environment is None:
        environment = {}
    env_action = '-'.join([f'{k}:{v}' for k, v in environment.items()])
    action_id = f'ssh-exec-{"-".join(cmd.split())}-{env_action}'
    handle_action(state, action_id)

    return None, StreamMock(), StreamMock()


def sftp_open(state, path, mode):
    """
    sftp.open mock
    """
    action_id = f'sftp-open-{path}-{mode}'
    handle_action(state, action_id)
    return StringIO()


def ssh(mocker, state):
    """
    Setup ssh mock
    """
    mocker.patch('paramiko.AutoAddPolicy')
    mocker.patch('cloud.mdb.dbaas_worker.internal.providers.ssh.paramiko.ecdsakey.ECDSAKey')
    env_get = mocker.patch('os.environ.get')
    env_get.return_value = None
    check_output = mocker.patch('subprocess.check_output')
    check_output.return_value = 'export SSH_AGENT_PID=0'.encode('utf-8')
    mocker.patch('subprocess.check_call')
    mocker.patch('os.kill')
    ssh_mock = mocker.patch('paramiko.SSHClient')
    ssh_mock.return_value.__enter__.return_value.exec_command.side_effect = lambda cmd, environment: exec_command(
        state, cmd, environment
    )

    ssh_mock.return_value.exec_command.side_effect = lambda cmd, environment: exec_command(state, cmd, environment)

    sftp_mock = mocker.patch('paramiko.SFTPClient.from_transport')
    sftp_mock.return_value.open.side_effect = lambda path, mode: sftp_open(state, path, mode)
