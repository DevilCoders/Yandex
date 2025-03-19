"""
Certificator interaction module
"""

import time
import OpenSSL

from dateutil import parser as dt_parser

from dbaas_common import retry, tracing

from cloud.mdb.dbaas_worker.internal.exceptions import ExposedException
from ..common import Change
from ..http import HTTPClient, HTTPErrorHandler
from .base import CrtClient, IssueResult


class CertificatorError(ExposedException):
    """
    Base certificator error
    """


class CertificatorApi(CrtClient, HTTPClient):
    """
    Certificator provider
    """

    def __init__(self, config, task, queue):
        super().__init__(config, task, queue)
        headers = {
            'Authorization': 'OAuth {token}'.format(token=self.config.cert_api.token),
        }
        self._init_session(
            self.config.cert_api.url,
            'api/certificate/',
            default_headers=headers,
            error_handler=HTTPErrorHandler(CertificatorError),
        )
        self.ca_name = self.config.cert_api.ca_name
        self.cert_type = self.config.cert_api.cert_type

    def _get_latest_cert(self, hostname, alt_names=None):
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

    @retry.on_exception(CertificatorError, factor=10, max_wait=60, max_tries=6)
    @tracing.trace('Certificator Issue')
    def issue(self, hostname, alt_names, force_tls_certs=False) -> IssueResult:
        """
        Issue new cert with certificator (or get existing)
        """
        tracing.set_tag('tls.cert.fqdn', hostname)
        tracing.set_tag('tls.cert.alt_names', alt_names)

        cert_info = {}
        if not force_tls_certs:
            cert_info = self._get_latest_cert(hostname, alt_names)
        if not cert_info:
            cert_info = self._issue_new(hostname, alt_names)

        return self._download_cert(cert_info)

    @retry.on_exception(CertificatorError, factor=10, max_wait=60, max_tries=6)
    @tracing.trace('Certificator Revoke')
    def revoke(self, hostname) -> None:
        """
        Revoke all issed certs for hostname
        """
        tracing.set_tag('tls.cert.fqdn', hostname)

        self.add_change(Change(f'tls-cert.{hostname}', 'revoke'))
        for cert in self._get_all_certs(hostname):
            if cert.get('status', 'issued') == 'issued' and cert.get('revoked') is None:
                self._revoke(cert)
