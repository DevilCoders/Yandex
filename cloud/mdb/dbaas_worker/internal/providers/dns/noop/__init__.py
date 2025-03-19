from ..client import BaseDnsClient, Record
from ...common import BaseProvider


class NoopDnsProvider(BaseDnsClient, BaseProvider):
    def set_records(self, fqdn: str, records: list[Record]) -> None:
        pass
