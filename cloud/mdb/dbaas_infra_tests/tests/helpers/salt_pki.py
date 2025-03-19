"""
Salt pki
"""

from cryptography.hazmat.backends import default_backend
from cryptography.hazmat.primitives import serialization
from cryptography.hazmat.primitives.asymmetric import rsa

from .utils import env_stage


class SaltPki:
    """
    Salt pki storage and generator
    """

    def __init__(self):
        self.storage = dict()

    def flush(self):
        """
        Drop storage
        """
        self.storage = dict()

    def gen_pair(self, name, key_size=4096):
        """
        Generate RSA-pair and store as name
        """
        if name not in self.storage:
            key = rsa.generate_private_key(public_exponent=65537, key_size=key_size, backend=default_backend())

            private = key.private_bytes(
                encoding=serialization.Encoding.PEM,
                format=serialization.PrivateFormat.TraditionalOpenSSL,
                encryption_algorithm=serialization.NoEncryption()).decode('utf-8')

            public = key.public_key().public_bytes(
                encoding=serialization.Encoding.PEM, format=serialization.PublicFormat.PKCS1).decode('utf-8')

            self.storage[name] = {
                'private': private,
                'public': public,
            }

        return self.storage[name]


@env_stage('create', fail=True)
def init_pki(*, state, **_):
    """
    Init pki
    """
    pki = state.get('salt_pki')
    if not pki:
        state['salt_pki'] = SaltPki()
