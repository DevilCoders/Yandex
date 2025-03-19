"""
DNS API interaction module
"""

from dbaas_common import retry, tracing
from dns.rdatatype import AAAA, A, RdataType
from dns.resolver import NXDOMAIN, NoAnswer, Resolver

from ....exceptions import ExposedException
from ...common import Change
from ...http import HTTPClient, HTTPErrorHandler
from ..client import Record, BaseDnsClient, ensure_dot, normalized_address


class DNSApiError(ExposedException):
    """
    Base dns error
    """


class SlayerDns(HTTPClient, BaseDnsClient):
    """
    DNS-API provider
    """

    def __init__(self, config, task, queue):
        super().__init__(config, task, queue)
        headers = {
            'X-Auth-Token': self.config.slayer_dns.token,
        }
        self._init_session(
            self.config.slayer_dns.url,
            f'/v2.3/{self.config.slayer_dns.account}/',
            default_headers=headers,
            error_handler=HTTPErrorHandler(DNSApiError),
            verify=self.config.slayer_dns.ca_path,
        )

        self.resolver = Resolver()
        self.resolver.nameservers = self.config.slayer_dns.name_servers

    @retry.on_exception(Exception, max_tries=3)
    def _resolve(self, fqdn: str, rdtype: RdataType) -> list[str]:
        """
        Resolve fqdn with resolver
        """
        ret = []
        try:
            res = self.resolver.query(ensure_dot(fqdn), rdtype)
            for answer in res.response.answer:
                for item in answer.items:
                    ret.append(item.address)
        except NXDOMAIN:
            self.logger.info('Got nxdomain for %s with type %s', fqdn, rdtype)
        except NoAnswer:
            self.logger.info('No records with type %s for %s', rdtype, fqdn)
        return ret

    def _get_records(self, fqdn: str) -> dict[str, str]:
        """
        Get all records for fqdn
        """
        ret = {}
        for record_type, rdtype in [('AAAA', AAAA), ('A', A)]:
            for record in self._resolve(fqdn, rdtype):
                ret[normalized_address(record, record_type)] = record_type
        return ret

    def _delete_record(self, fqdn, address, record_type):
        """
        Remove record
        """
        self._make_request(
            'primitives',
            'put',
            data={
                'primitives': [
                    {
                        'operation': 'delete',
                        'name': fqdn,
                        'data': address,
                        'type': record_type,
                        'ttl': self.config.slayer_dns.ttl,
                    }
                ],
            },
        )

    def _add_record(self, fqdn, address, record_type):
        """
        Create record for fqdn with address and type
        """
        self._make_request(
            'primitives',
            'put',
            data={
                'primitives': [
                    {
                        'operation': 'add',
                        'name': fqdn,
                        'type': record_type,
                        'data': address,
                        'ttl': self.config.slayer_dns.ttl,
                    }
                ],
            },
        )

    def _gen_add_rollback(self, fqdn, address, record_type):
        """
        Create rollback function for record add (we can not use lambda directly because record is a loop var)
        """
        return lambda task, safe_revision: self._delete_record(fqdn, address, record_type)

    def _gen_delete_rollback(self, fqdn, address, record_type):
        """
        Create rollback function for record delete.
        """
        return lambda task, safe_revision: self._add_record(fqdn, address, record_type)

    @retry.on_exception_fixed_interval(DNSApiError, interval=30, max_tries=12)
    @tracing.trace('Slayer DNS Set Records')
    def set_records(self, fqdn: str, records: list[Record]) -> None:
        """
        CAS-style update of records for fqdn
        """
        tracing.set_tag('dns.fqdn', fqdn)
        tracing.set_tag('dns.records', records)

        existing = self._get_records(fqdn)
        delete = []
        expected_map = {normalized_address(x.address, x.record_type): x for x in records}

        for address, record_type in existing.items():
            if address not in expected_map:
                delete.append((address, record_type))

        for record in expected_map.values():
            self.add_change(
                Change(
                    f'dns.{fqdn}-{record.record_type}-{record.address}',
                    'created',
                    rollback=self._gen_add_rollback(fqdn, record.address, record.record_type),
                )
            )
            self._add_record(fqdn, record.address, record.record_type)

        for address, record_type in delete:
            self.add_change(
                Change(
                    f'dns.{fqdn}-{record_type}-{address}',
                    'removed',
                    rollback=self._gen_delete_rollback(fqdn, address, record_type),
                )
            )
            self._delete_record(fqdn, address, record_type)
