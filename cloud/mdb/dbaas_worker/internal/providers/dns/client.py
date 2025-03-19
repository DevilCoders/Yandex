from abc import ABC, abstractmethod
from typing import NamedTuple
from ipaddress import IPv6Address
import json
import hashlib


def ensure_dot(hostname: str) -> str:
    if not hostname.endswith('.'):
        hostname += '.'
    return hostname


def normalized_address(address: str, record_type: str) -> str:
    return str(IPv6Address(address)) if record_type == 'AAAA' else address


class Record(NamedTuple):
    address: str
    record_type: str


def records_stable_hash(records: list[Record]) -> str:
    """
    https://death.andgravity.com/stable-hashing
    """
    return hashlib.md5(
        json.dumps(
            list(sorted(records)),
            ensure_ascii=False,
            sort_keys=True,
            indent=None,
            separators=(',', ':'),
        ).encode()
    ).hexdigest()


class BaseDnsClient(ABC):
    @abstractmethod
    def set_records(self, fqdn: str, records: list[Record]) -> None:
        pass
