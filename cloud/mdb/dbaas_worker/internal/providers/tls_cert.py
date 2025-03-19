"""
TLS cert/key pillar manipulation
"""

from functools import partial

from dbaas_common import tracing

from ..crypto import encrypt
from .certificator import CertificatorApi
from .certificator.base import AvailableCerts
from .common import BaseProvider, Change
from .pillar import DbaasPillar


class TLSCert(BaseProvider):
    """
    TLSCert provider
    """

    def __init__(self, config, task, queue):
        super().__init__(config, task, queue)
        self.wildcard_cert = config.cert_api.wildcard_cert
        self.pillar = DbaasPillar(config, task, queue)
        self.cert_api = CertificatorApi(config, task, queue)
        if config.cert_api.api == AvailableCerts.certificator:
            self.save_in_db = True
        else:
            self.save_in_db = False

    def _issue(self, fqdn, alt_names, force_tls_certs=False):
        """
        Call cert_api issue
        """
        if self.wildcard_cert:
            fqdn = '.'.join(['*'] + fqdn.split('.')[1:])
        return self.cert_api.issue(fqdn, alt_names, force_tls_certs)

    def _issue_and_encrypt(self, fqdn, alt_names, force_tls_certs=False):
        """
        Call cert_api issue and encrypt result
        """
        key, cert, expiration = self._issue(fqdn, alt_names, force_tls_certs)
        ekey = encrypt(self.config, key)
        ecert = encrypt(self.config, cert)
        return ekey, ecert, expiration

    def _issue_expiration(self, fqdn, alt_names, force_tls_certs):
        _, _, expiration = self._issue(fqdn, alt_names, force_tls_certs)
        return expiration

    @tracing.trace('TLS Certs Exists')
    def exists(self, fqdn, alt_names=None, force=False, force_tls_certs=False):
        """
        Ensure that TLS key and cert are in pillar
        """
        tracing.set_tag('tls.cert.fqdn', fqdn)
        tracing.set_tag('tls.cert.alt_names', alt_names)

        self.add_change(
            Change(f'tls-cert.{fqdn}', 'issued', rollback=lambda task, safe_refision: self.cert_api.revoke(fqdn))
        )

        if self.save_in_db:
            self.pillar.exists(
                'fqdn',
                fqdn,
                [],
                ('cert.key', 'cert.crt', 'cert.expiration'),
                partial(self._issue_and_encrypt, fqdn, alt_names, force_tls_certs),
                force=force,
            )
        else:
            self.pillar.remove(
                'fqdn',
                fqdn,
                [],
                ('cert.key', 'cert.crt', 'cert.expiration'),
            )
            expiration = self._issue_expiration(fqdn, alt_names, force_tls_certs)
            self.pillar.exists(
                'fqdn',
                fqdn,
                [],
                ('cert.expiration',),
                (expiration,),
                force=force,
            )
