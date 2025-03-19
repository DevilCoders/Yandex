from typing import Type
from .client import BaseDnsClient, Record
from . import slayer, yc, noop


class DnsApi:
    """
    Depending on config instantiate different Dns providers
    """

    __clients: dict[str, Type[BaseDnsClient]] = {
        'SLAYER': slayer.SlayerDns,
        'YC.DNS': yc.YCDns,
        'NOOP': noop.NoopDnsProvider,
    }

    def __init__(self, config, task, queue):
        if config.dns.api not in self.__clients:
            raise RuntimeError(f'Unsupported dns provider {config.dns.api}')
        # Silent mypy here, cause DnsClient ABC doesn't have 'provider' constructor.
        # We can add it, but dealing with  multiple inheritance from BaseProvider looks more painful.
        self._client: BaseDnsClient = self.__clients[config.dns.api](config, task, queue)  # type: ignore

    def set_records(self, fqdn: str, records: list[Record]) -> None:
        self._client.set_records(fqdn, records)
