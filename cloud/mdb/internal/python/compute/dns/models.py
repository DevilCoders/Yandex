from typing import Any, NamedTuple

from yandex.cloud.priv.dns.v1 import dns_zone_pb2


class DnsRecordSet(NamedTuple):
    name: str
    type: str
    ttl: int
    data: list[str]

    @staticmethod
    def from_api(rec: Any) -> 'DnsRecordSet':  # type: ignore
        return DnsRecordSet(
            name=rec.name,
            type=rec.type,
            ttl=rec.ttl,
            data=list(rec.data),
        )

    def to_api(self):
        # mypy still useless with generated proto
        # ....
        # error: dns_zone_pb2.RecordSet? has no attribute "name"
        #
        # type: ignore
        return dns_zone_pb2.RecordSet(
            name=self.name,
            type=self.type,
            ttl=self.ttl,
            data=self.data,
        )
