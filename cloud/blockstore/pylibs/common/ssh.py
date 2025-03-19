from .retry import retry

from contextlib import contextmanager

import paramiko
import sys

_SSH_DEFAULT_RETRIES_COUNT = 15  # seconds
_SSH_DEFAULT_USER = 'root'


class SshTestClient:

    class FakeStdout:

        class FakeChannel:
            def exec_command(self, command):
                sys.stdout.write(f'Execute command {command}')

            def recv_exit_status(self):
                return 0

            def exit_status_ready(self):
                return 1

        def __init__(self):
            self.channel = SshTestClient.FakeStdout.FakeChannel()

        def readlines(self):
            return []

        def read(self):
            return b''

    class FakeTransport:
        def __init__(self, ip: str):
            self.ip = ip

        def open_session(self):
            sys.stdout.write(f'Open SSH session {self.ip}')
            return SshTestClient.FakeStdout.FakeChannel()

    def __init__(self, ip: str):
        self.ip = ip

    def exec_command(self, command):
        sys.stdout.write(f'SSH {self.ip}: {command}\n')
        return None, SshTestClient.FakeStdout(), SshTestClient.FakeStdout()

    def get_transport(self):
        return SshTestClient.FakeTransport(self.ip)

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        pass


class SshClient:
    def __init__(self, ip: str, user: str, timeout: int):
        self.client = ssh_client(ip, user, timeout)

    @retry(10, 60)
    def exec_command(self, command):
        return self.client.exec_command(command)

    @retry(10, 60)
    def get_transport(self):
        return self.client.get_transport()

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        pass


class SftpTestClient:

    class SftpTestFile:
        def __init__(self, ip):
            self.ip = ip

        def read(self):
            sys.stdout.write(f'SFTP READ FILE {self.ip}')
            return 'text'

        def write(self, text):
            sys.stdout.write(f'SFTP WRITE FILE {self.ip}')

        def flush(self):
            sys.stdout.write(f'SFTP FLUSH FILE {self.ip}')

        def readlines(self):
            return self.read()

    def __init__(self, ip):
        self.ip = ip

    def put(self, src, dst):
        sys.stdout.write(f'SFTP PUT {self.ip}/{src} -> {dst}\n')

    def chmod(self, dst, flags="r"):
        sys.stdout.write(f'SFTP CHMOD {self.ip}/{dst} f={flags}\n')

    def file(self, dst, flags="r"):
        sys.stdout.write(f'SFTP FILE {self.ip}/{dst} f={flags}\n')
        return SftpTestClient.SftpTestFile(self.ip)

    def mkdir(self, path):
        sys.stdout.write(f'SFTP MKDIR {self.ip} dir={path}\n')

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        pass


def configure_ssh_client(
    client: paramiko.SSHClient,
    ip: str,
    user: str = _SSH_DEFAULT_USER,
    retries: int = _SSH_DEFAULT_RETRIES_COUNT
):
    client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    client.load_system_host_keys()
    retry_func = retry(tries=retries, delay=60)
    retry_func(client.connect)(ip, username=user, timeout=180, password='')
    transport = client.get_transport()
    transport.set_keepalive(60)
    session = transport.open_session()
    paramiko.agent.AgentRequestHandler(session)


@contextmanager
def ssh_client(
    ip: str,
    user: str = _SSH_DEFAULT_USER,
    retries: int = _SSH_DEFAULT_RETRIES_COUNT
) -> paramiko.SSHClient:
    with paramiko.SSHClient() as ssh_client:
        configure_ssh_client(ssh_client, ip, user, retries)
        yield ssh_client


def make_channel(
    dry_run: bool,
    ip: str,
    user: str = _SSH_DEFAULT_USER,
    retries: int = _SSH_DEFAULT_RETRIES_COUNT
):
    if dry_run:
        return SshTestClient.FakeStdout().channel

    ssh_client = paramiko.SSHClient()
    configure_ssh_client(ssh_client, ip, user, retries)
    return ssh_client.get_transport().open_session()


def make_ssh_client(
    dry_run: bool,
    ip: str,
    user: str = _SSH_DEFAULT_USER,
    retries: int = _SSH_DEFAULT_RETRIES_COUNT
):
    if dry_run:
        return SshTestClient(ip)

    return ssh_client(ip, user, retries)


@contextmanager
def sftp_client(
    ip: str,
    user: str = _SSH_DEFAULT_USER,
    retries: int = _SSH_DEFAULT_RETRIES_COUNT
) -> paramiko.SFTPClient:
    with ssh_client(ip, user, retries) as ssh:
        yield ssh.open_sftp()


def make_sftp_client(
    dry_run: bool,
    ip: str,
    user: str = _SSH_DEFAULT_USER,
    retries: int = _SSH_DEFAULT_RETRIES_COUNT
):
    if dry_run:
        return SftpTestClient(ip)

    return sftp_client(ip, user, retries)
