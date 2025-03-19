from abc import ABC, abstractmethod
from typing import Any, NamedTuple


class AvailableCerts:
    certificator = 'CERTIFICATOR'
    mdb_secrets = 'MDB_SECRETS'
    noop = 'NOOP'


class IssueResult(NamedTuple):
    key: Any
    cert: str
    expiration: str


class CrtClient(ABC):
    @abstractmethod
    def issue(self, hostname, alt_names, force_tls_certs=False) -> IssueResult:
        """
        Issue certificate.
        """

    @abstractmethod
    def revoke(self, hostname) -> None:
        """
        Revoke certificate.
        """
