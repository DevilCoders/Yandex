"""
SSH pki
"""

from cryptography.hazmat.backends import default_backend
from cryptography.hazmat.primitives import serialization
from cryptography.hazmat.primitives.asymmetric import ec

from .utils import env_stage


class SshPki:
    """
    Ssh pki storage and generator
    """

    def __init__(self):
        self.storage = dict()

    def flush(self):
        """
        Drop storage
        """
        self.storage = dict()

    def gen_pair(self, name):
        """
        Generate RSA-pair and store as name
        """
        if name not in self.storage:
            key = ec.generate_private_key(curve=ec.SECP521R1(), backend=default_backend())

            private = key.private_bytes(
                encoding=serialization.Encoding.PEM,
                format=serialization.PrivateFormat.PKCS8,
                encryption_algorithm=serialization.NoEncryption()).decode('utf-8')

            public = key.public_key().public_bytes(
                encoding=serialization.Encoding.OpenSSH, format=serialization.PublicFormat.OpenSSH).decode('utf-8')

            self.storage[name] = {
                'private': private.replace(' PRIVATE KEY', ' EC PRIVATE KEY'),
                'public': '{key} {name}'.format(key=public, name=name),
            }

        return self.storage[name]


@env_stage('create', fail=True)
def init_pki(*, state, **_):
    """
    Init pki
    """
    pki = state.get('ssh_pki')
    if not pki:
        state['ssh_pki'] = SshPki()
