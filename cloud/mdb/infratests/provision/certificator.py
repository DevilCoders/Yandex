"""
Certificator interaction module
"""

import json
import logging
import OpenSSL
import requests
import time
import urllib.parse
import uuid

from copy import deepcopy
from dateutil import parser as dt_parser
from json.decoder import JSONDecodeError
from typing import Any, List, NamedTuple

from dbaas_common import retry

from cloud.mdb.infratests.config import InfratestConfig


class IssueResult(NamedTuple):
    key: Any
    cert: str
    expiration: str


class CertificatorError(RuntimeError):
    """
    Base certificator error
    """


class CertificatorApi:
    """
    Certificator provider
    """

    def __init__(self, config: InfratestConfig, logger: logging.Logger):
        self.default_headers = {
            'Authorization': 'OAuth {token}'.format(token=config.certificator.token),
        }
        self.logger = logger
        self.verify = True
        self.error_handler = HTTPErrorHandler(CertificatorError)
        self.internal_error_handler = HTTPErrorHandler(requests.exceptions.HTTPError)

        self.ca_name = config.certificator.ca_name
        self.cert_type = config.certificator.cert_type

        self.base_url = config.certificator.url
        if not self.base_url.endswith('/'):
            self.base_url = f'{self.base_url}/'
        self.base_url = urllib.parse.urljoin(self.base_url, 'api/certificate/'.lstrip('/'))
        self.session = requests.Session()
        adapter = requests.adapters.HTTPAdapter(pool_connections=1, pool_maxsize=1)
        parsed = urllib.parse.urlparse(self.base_url)
        self.session.mount(f'{parsed.scheme}://{parsed.netloc}', adapter)

    @retry.on_exception(CertificatorError, factor=10, max_wait=60, max_tries=6)
    def issue(self, hostname: str, alt_names: List[str], force_tls_certs=False) -> IssueResult:
        """
        Issue new cert with certificator (or get existing)
        """
        cert_info = {}
        if not force_tls_certs:
            cert_info = self._get_latest_cert(hostname, alt_names)
        if not cert_info:
            cert_info = self._issue_new(hostname, alt_names)

        return self._download_cert(cert_info)

    @retry.on_exception(CertificatorError, factor=10, max_wait=60, max_tries=6)
    def revoke(self, hostname) -> None:
        """
        Revoke all issued certs for hostname
        """
        for cert in self._get_all_certs(hostname):
            if cert.get('status', 'issued') == 'issued' and cert.get('revoked') is None:
                self._revoke(cert)

    def _get_latest_cert(self, hostname: str, alt_names: List[str] = None) -> dict:
        """
        Find latest cert in certificator with not expired key
        """
        cert_info = {}
        max_issued = 0
        now = time.time()
        hosts = [hostname]
        if alt_names:
            hosts += alt_names
        for result in self._get_all_certs(hostname):
            if result.get('status') == 'issued' and result.get('revoked') is None:
                issue_ts = dt_parser.parse(result['issued']).timestamp()
                end_ts = dt_parser.parse(result['end_date']).timestamp()
                if (
                    end_ts - 30 * 24 * 3600 > now
                    and issue_ts > max_issued
                    and not result.get('priv_key_deleted_at')
                    and result.get('hosts') == hosts
                ):
                    cert_info = result
                    max_issued = issue_ts

        return cert_info

    def _get_all_certs(self, hostname):
        """
        Get all certs for hostname
        """
        res = self._make_request('', params={'host': hostname})
        return res.get('results', [])

    def _issue_new(self, hostname, alt_names):
        """
        Issue new cert with specified CA
        """
        hosts = [hostname]
        if alt_names:
            hosts += alt_names
        issue_data = {
            'type': self.cert_type,
            'hosts': ','.join(hosts),
            'ca_name': self.ca_name,
        }

        return self._make_request('', 'post', expect=[201], data=issue_data)

    def _download_cert(self, cert_info) -> IssueResult:
        """
        Get cert and key from certificator
        """
        if 'download2' not in cert_info:
            raise CertificatorError('Download url not found in {cert}'.format(cert=cert_info))
        res = self._make_request('{url}.pem'.format(url=cert_info['download2'])).text
        key = ''
        key_lines = False
        cert = ''
        for line in res.split('\n'):
            if 'PRIVATE KEY--' in line:
                if '--BEGIN' in line:
                    key_lines = True
                    key += line + '\n'
                elif '--END' in line:
                    key_lines = False
                    key += line + '\n'
            elif key_lines:
                key += line + '\n'
            else:
                cert += line + '\n'

        if not key:
            raise CertificatorError('No private key in: {res}'.format(res=res))

        x509 = OpenSSL.crypto.load_certificate(OpenSSL.crypto.FILETYPE_PEM, cert)
        # YYYYMMDDhhmmssZ
        expiration_openssl_format = x509.get_notAfter()

        # YYYY-MM-DDThh:mm:ssZ
        expiration_swagger_format = '{Y}-{M}-{D}T{h}:{m}:{s}Z'.format(
            Y=expiration_openssl_format[0:4].decode('utf-8'),
            M=expiration_openssl_format[4:6].decode('utf-8'),
            D=expiration_openssl_format[6:8].decode('utf-8'),
            h=expiration_openssl_format[8:10].decode('utf-8'),
            m=expiration_openssl_format[10:12].decode('utf-8'),
            s=expiration_openssl_format[12:14].decode('utf-8'),
        )
        return IssueResult(key, cert, expiration_swagger_format)

    def _revoke(self, cert_info):
        """
        Revoke issued cert
        """
        if 'url' not in cert_info:
            raise CertificatorError('Cert url not found in {cert}'.format(cert=cert_info))
        res = self._make_request(cert_info['url'], 'delete')
        if not res.get('revoked'):
            raise CertificatorError('Unable to revoke cert with url {url}'.format(url=cert_info['url']))

    @retry.on_exception((JSONDecodeError, requests.exceptions.RequestException), factor=1, max_wait=5, max_tries=24)
    def _make_request(self, path, method='get', expect=None, data=None, params=None, headers=None, verify=None):
        """
        Make http request
        """
        if expect is None:
            expect = [200]
        if path.startswith('http'):
            url = path
        else:
            url = urllib.parse.urljoin(self.base_url, path)

        default_headers = self.default_headers() if callable(self.default_headers) else self.default_headers.copy()

        kwargs = {
            'method': method,
            'url': url,
            'headers': default_headers,
            'timeout': (1, 600),
            'verify': self.verify if verify is None else verify,
            'params': params,
        }

        if headers:
            kwargs['headers'].update(headers)

        if data is not None:
            kwargs['headers']['Accept'] = 'application/json'
            kwargs['headers']['Content-Type'] = 'application/json'
            kwargs['json'] = data

        request_id = str(uuid.uuid4())
        kwargs['headers']['X-Request-Id'] = request_id

        extra = {'request_id': request_id}

        self.logger.info(
            'Starting %s request to %s%s',
            method.upper(),
            _get_full_url(kwargs['url'], params),
            f' with {_remove_secret_fields(data)}' if data else '',
            extra=extra,
        )

        res = self.session.request(**kwargs)

        self.logger.info(
            '%s request to %s finished with %s',
            method.upper(),
            _get_full_url(kwargs['url'], params),
            res.status_code,
            extra=extra,
        )

        if res.status_code >= 500:
            self.internal_error_handler.handle(kwargs, request_id, res)

        if res.status_code not in expect:
            self.error_handler.handle(kwargs, request_id, res)

        if 'application/json' in res.headers.get('Content-Type', '').lower():
            return res.json()

        return res


def _get_full_url(url, params):
    """
    Get curl-friendly url from url and params
    """
    if params:
        return f'{url}?{urllib.parse.urlencode(params)}'
    return url


SECRET_FIELDS = [
    'secret',
    'secrets',
]


def _remove_secret_fields(data):
    """
    Remove secrets from http call (for safe logging)
    """
    data_copy = deepcopy(data)
    for field in SECRET_FIELDS:
        if field in data_copy:
            data_copy[field] = '***'
    return json.dumps(data_copy)


class HTTPErrorHandler:
    """
    HTTP error handler
    """

    def __init__(self, error_type):
        self.error_type = error_type

    def handle(self, kwargs, request_id, result):
        """
        Raise exception from http result
        """
        raise self.error_type(
            f'Unexpected {kwargs["method"].upper()} '
            f'{_get_full_url(kwargs["url"], kwargs["params"])} '
            f'result: {result.status_code} {result.text} '
            f'request_id: {request_id}'
        )
