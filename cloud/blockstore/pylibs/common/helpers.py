from .retry import retry
from .ssh import sftp_client, ssh_client

import ssl
from subprocess import CompletedProcess
from subprocess import PIPE
from subprocess import run
import sys
import tempfile
import urllib3
import uuid

_DEFAULT_SSH_USER = 'root'


class Helpers:

    def wait_until_instance_becomes_available_via_ssh(
        self,
        ip: str,
        user: str = _DEFAULT_SSH_USER
    ) -> None:
        # Reconnect always
        @retry(tries=5, delay=60)
        def _connect():
            with ssh_client(ip, user=user):
                pass

        _connect()

    def wait_for_block_device_to_appear(
        self,
        ip: str,
        path: str,
        user: str = _DEFAULT_SSH_USER
    ) -> None:
        with sftp_client(ip, user=user) as sftp:
            @retry(tries=5, delay=60)
            def _stat():
                sftp.stat(path)

            _stat()

    def make_get_request(self, url: str) -> int:
        urllib3.disable_warnings(urllib3.exceptions.InsecureRequestWarning)
        http = urllib3.PoolManager(cert_reqs=ssl.CERT_NONE)
        r = http.request('GET', url)
        return r.status

    def make_subprocess_run(self, command) -> CompletedProcess:
        result = run(command, stdout=PIPE, stderr=PIPE, universal_newlines=True, shell=True)
        return result

    def generate_id(self):
        return uuid.uuid1()

    def create_tmp_file(self):
        tmp_file = tempfile.NamedTemporaryFile(suffix=".tmp")
        return tmp_file


class TestHelpers:

    class TmpFile:

        def __init__(self, id):
            self.name = f'{id}.tmp'
            sys.stdout.write(f'Create tmp file with name=<{self.name}>\n')

        def write(self, text: str):
            sys.stdout.write(f'Write to tmp file with name=<{self.name}>\n')

        def flush(self):
            sys.stdout.write(f'Flush tmp file with name=<{self.name}>\n')

    def __init__(self):
        self._id = 0

    def wait_until_instance_becomes_available_via_ssh(
        self,
        ip: str,
        user: str = _DEFAULT_SSH_USER
    ) -> None:
        sys.stdout.write(f'Waiting for instance {ip}\n')

    def wait_for_block_device_to_appear(
        self,
        ip: str,
        path: str,
        user: str = _DEFAULT_SSH_USER
    ) -> None:
        sys.stdout.write(f'Waiting for bdev {ip}/{path}\n')

    def make_get_request(self, url: str) -> int:
        sys.stdout.write(f'GET request url=<{url}>\n')
        return 200

    def make_subprocess_run(self, command) -> CompletedProcess:
        sys.stdout.write(f'Execute subprocess.run command=<{command}>\n')
        return CompletedProcess(command, 0)

    def generate_id(self):
        self._id += 1
        return self._id

    def create_tmp_file(self):
        self._id += 1
        return TestHelpers.TmpFile(self._id)


def make_helpers(dry_run):
    return TestHelpers() if dry_run else Helpers()
