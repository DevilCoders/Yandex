"""
SQLServer replication certificate manipulation
"""

import datetime
from functools import partial

from cryptography.hazmat.backends import default_backend
from cryptography.hazmat.primitives import serialization
from cryptography.hazmat.primitives.asymmetric import rsa
from cryptography import x509
from cryptography.x509 import CertificateBuilder, random_serial_number
from cryptography.x509.oid import NameOID
from cryptography.hazmat.primitives import hashes

from ..crypto import encrypt
from .common import BaseProvider
from .pillar import DbaasPillar


class SQLServerReplCertProvider(BaseProvider):
    """
    Provides self-signed certificate to authorize SQLServer nodes for replication
    """

    def __init__(self, config, task, queue):
        super().__init__(config, task, queue)
        self.pillar = DbaasPillar(config, task, queue)

    def _generate(self, key_size=2048, expire_days=50 * 365):
        key = rsa.generate_private_key(public_exponent=65537, key_size=key_size, backend=default_backend())

        key_text = key.private_bytes(
            encoding=serialization.Encoding.PEM,
            format=serialization.PrivateFormat.PKCS8,
            encryption_algorithm=serialization.NoEncryption(),
        ).decode('utf-8')

        subject = issuer = x509.Name(
            [
                x509.NameAttribute(NameOID.COUNTRY_NAME, u"RU"),
                x509.NameAttribute(NameOID.STATE_OR_PROVINCE_NAME, u"Moscow"),
                x509.NameAttribute(NameOID.LOCALITY_NAME, u"Moscow"),
                x509.NameAttribute(NameOID.ORGANIZATION_NAME, u"Yandex"),
                x509.NameAttribute(NameOID.COMMON_NAME, u"repl.local"),
            ]
        )

        cert = (
            CertificateBuilder()
            .subject_name(subject)
            .issuer_name(issuer)
            .public_key(key.public_key())
            .serial_number(random_serial_number())
            .not_valid_before(datetime.datetime.utcnow())
            .not_valid_after(datetime.datetime.utcnow() + datetime.timedelta(days=expire_days))
            .sign(key, hashes.SHA256(), backend=default_backend())
        )

        cert_text = cert.public_bytes(serialization.Encoding.PEM).decode('utf-8')

        return (encrypt(self.config, cert_text), encrypt(self.config, key_text))

    def exists(self, cid):
        """
        Ensure that certificate for sqlserver replication exists
        """
        self.pillar.exists(
            'cid',
            cid,
            ['data', 'sqlserver', 'repl_cert'],
            ['cert.crt', 'cert.key'],
            partial(self._generate),
        )
