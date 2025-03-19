import json
import subprocess

from click import ClickException


class VaultError(Exception):
    def __init__(self, command, stderr, stdout):
        message = f'`{command}` failed'
        if stderr:
            message += ' with: ' + stderr.decode()
        elif stdout:
            message += ' with: ' + stdout.decode()

        super().__init__(message)
        self.command = command
        try:
            self.reponse = json.loads(stderr.decode())
        except Exception:
            self.reponse = {}

    @property
    def code(self):
        return self.reponse.get('code')


def get_secret(secret):
    try:
        return _execute(f'get version --json "{secret}"')['value']
    except VaultError as e:
        if e.code == 'access_error':
            raise ClickException(f'Not enough permissions to get the secret "{secret}".')
        raise


def _execute(command):
    command = f'ya vault {command}'

    proc = subprocess.Popen(command, shell=True, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

    stdout, stderr = proc.communicate()

    if proc.returncode:
        raise VaultError(command, stderr, stdout)

    return json.loads(stdout.decode())
