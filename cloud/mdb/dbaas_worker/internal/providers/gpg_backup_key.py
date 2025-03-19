"""
GPG backup key pillar manipulation
"""

from functools import partial

import pgpy
from pgpy.constants import CompressionAlgorithm, HashAlgorithm, KeyFlags, PubKeyAlgorithm, SymmetricKeyAlgorithm

from ..crypto import encrypt
from .common import BaseProvider
from .pillar import DbaasPillar


class GpgBackupKey(BaseProvider):
    """
    GPG backup key provider
    """

    def __init__(self, config, task, queue):
        super().__init__(config, task, queue)
        self.pillar = DbaasPillar(config, task, queue)

    def _generate(self, name, email='mdb-admin@yandex-team.ru'):
        key = pgpy.PGPKey.new(PubKeyAlgorithm.RSAEncryptOrSign, 4096)
        uid = pgpy.PGPUID.new(name, comment=name, email=email)

        key.add_uid(
            uid,
            usage={
                KeyFlags.Sign,
                KeyFlags.EncryptCommunications,
                KeyFlags.EncryptStorage,
            },
            hashes=[
                HashAlgorithm.SHA256,
                HashAlgorithm.SHA384,
                HashAlgorithm.SHA512,
                HashAlgorithm.SHA224,
            ],
            ciphers=[
                SymmetricKeyAlgorithm.AES256,
                SymmetricKeyAlgorithm.AES192,
                SymmetricKeyAlgorithm.AES128,
            ],
            compression=[
                CompressionAlgorithm.ZLIB,
                CompressionAlgorithm.BZ2,
                CompressionAlgorithm.ZIP,
                CompressionAlgorithm.Uncompressed,
            ],
        )

        return encrypt(self.config, str(key)), key.fingerprint.keyid

    def exists(self, cid):
        """
        Ensure that GPG key for backups exists
        """
        self.pillar.exists('cid', cid, ['data', 's3'], ['gpg_key', 'gpg_key_id'], partial(self._generate, cid))
