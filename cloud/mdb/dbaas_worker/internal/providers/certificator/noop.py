"""
Noop Certificator
"""
from datetime import datetime, timedelta
from .base import CrtClient, IssueResult


class Noop(CrtClient):
    def issue(self, hostname, alt_names, force_tls_certs=False) -> IssueResult:
        """
        Issue certificate.
        """
        expire_at = datetime.utcnow() + timedelta(days=360)
        return IssueResult(
            f'Noop-Certificate-Key-for-{hostname}',
            f'Noop-Certificate-Cert-for-{hostname}',
            expire_at.strftime('%Y-%m-%dT%H:%M:%SZ'),
        )

    def revoke(self, hostname) -> None:
        """
        Revoke certificate.
        """
        return None
