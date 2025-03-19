"""
DNS API interaction module
"""

import json
import urllib.parse
from ipaddress import IPv6Address
from typing import List, NamedTuple

import requests
from dns.rdatatype import AAAA, A
from dns.resolver import NXDOMAIN, NoAnswer, Resolver
from retrying import retry

Record = NamedTuple('Record', [('address', str), ('record_type', str)])


class DNSApiError(Exception):
    """
    Base dns error
    """


class DnsApi:
    """
    DNS provider
    """

    def __init__(self, config, logger):
        self.config = config
        self.logger = logger
        self.base_url = urllib.parse.urljoin(
            self.config['url'], '/v2.3/{account}/'.format(account=self.config['account'])
        )
        self.headers = {
            'X-Auth-Token': self.config['token'],
            'Accept': 'application/json',
            'Content-Type': 'application/json',
        }
        self.resolver = Resolver()
        self.resolver.nameservers = [self.config['name_server']]

    @retry(wait_fixed=5000, stop_max_attempt_number=6)
    def _make_request(self, path, method='put', expect=None, data=None):
        """
        Make request to dns-api
        """
        if expect is None:
            expect = [200]
        kwargs = {
            'method': method,
            'url': urllib.parse.urljoin(self.base_url, path.lstrip('/')),
            'headers': self.headers,
            'timeout': (1, 30),
            'verify': self.config['ca_path'],
        }
        if data:
            kwargs['data'] = json.dumps(data)
        res = requests.request(**kwargs)
        self.logger.info('%s %s %s %s', method.upper(), kwargs['url'], kwargs['data'] if data else '-', res.status_code)
        if res.status_code not in expect:
            raise DNSApiError(f'Unexpected {method.upper()} {kwargs["url"]} result: {res.status_code} {res.text}')
        return res

    @retry(wait_fixed=5000, stop_max_attempt_number=3)
    def _resolve(self, fqdn, rdtype):
        """
        Resolve fqdn with resolver
        """
        ret = []
        try:
            res = self.resolver.query(fqdn, rdtype)
            for answer in res.response.answer:
                for item in answer.items:
                    ret.append(item.address)
        except NXDOMAIN:
            self.logger.info('Got nxdomain for %s with type %s', fqdn, rdtype)
        except NoAnswer:
            self.logger.info('No records with type %s for %s', rdtype, fqdn)
        return ret

    def _get_records(self, fqdn):
        """
        Get all records for fqdn
        """
        ret = {}
        for record_type, rdtype in [('AAAA', AAAA), ('A', A)]:
            for record in self._resolve(fqdn, rdtype):
                if rdtype == AAAA:
                    ret[str(IPv6Address(record))] = record_type
                else:
                    ret[record] = record_type
        return ret

    def _delete_record(self, fqdn, address, record_type):
        """
        Remove record
        """
        self._make_request(
            'primitives',
            data={
                'primitives': [
                    {
                        'operation': 'delete',
                        'name': fqdn,
                        'data': address,
                        'type': record_type,
                        'ttl': self.config['ttl'],
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
            data={
                'primitives': [
                    {
                        'operation': 'add',
                        'name': fqdn,
                        'type': record_type,
                        'data': address,
                        'ttl': self.config['ttl'],
                    }
                ],
            },
        )

    @retry(wait_fixed=5000, stop_max_attempt_number=6)
    def set_records(self, fqdn: str, records: List[Record]):
        """
        CAS-style update of records for fqdn
        """
        existing = self._get_records(fqdn)
        delete = []
        create = []
        expected_map = {str(IPv6Address(x.address)) if x.record_type == 'AAAA' else x.address: x for x in records}

        for address, record_type in existing.items():
            if address not in expected_map:
                delete.append((address, record_type))

        for expected, data in expected_map.items():
            if expected not in existing:
                create.append(data)

        for record in create:
            self._add_record(fqdn, record.address, record.record_type)

        for address, record_type in delete:
            self._delete_record(fqdn, address, record_type)
