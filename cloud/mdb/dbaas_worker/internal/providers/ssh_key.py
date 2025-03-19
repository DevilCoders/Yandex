"""
Ssh key generator
"""

from cryptography.hazmat.backends import default_backend
from cryptography.hazmat.primitives import serialization
from cryptography.hazmat.primitives.asymmetric import rsa

from ..crypto import encrypt
from .common import BaseProvider


class SshKeyProvider(BaseProvider):
    """
    Ssh keys provider
    """

    def generate(self, key_size=2048, private_format='PKCS8'):
        key = rsa.generate_private_key(public_exponent=65537, key_size=key_size, backend=default_backend())

        private = key.private_bytes(
            encoding=serialization.Encoding.PEM,
            format=getattr(serialization.PrivateFormat, private_format),
            encryption_algorithm=serialization.NoEncryption(),
        ).decode('utf-8')

        public = (
            key.public_key()
            .public_bytes(encoding=serialization.Encoding.OpenSSH, format=serialization.PublicFormat.OpenSSH)
            .decode('utf-8')
        )

        return (encrypt(self.config, public), encrypt(self.config, private))
