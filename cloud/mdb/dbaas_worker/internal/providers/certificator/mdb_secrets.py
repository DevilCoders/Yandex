"""
MDB Secrets interaction module
"""

from dbaas_common import retry, tracing

from cloud.mdb.dbaas_worker.internal.exceptions import ExposedException
from ..common import Change
from ..iam_jwt import IamJwt
from ..http import HTTPClient, HTTPErrorHandler
from .base import CrtClient, IssueResult


class MDBSecretsError(ExposedException):
    """
    Base MDB Secrets error
    """


class MDBSecretsApi(CrtClient, HTTPClient):
    """
    MDBSecrets provider
    """

    def __init__(self, config, task, queue):
        super().__init__(config, task, queue)
        self.iam_jwt = None
        if not self.config.cert_api.token:
            self.iam_jwt = IamJwt(config, task, queue)
        self._init_session(
            self.config.cert_api.url,
            'v1/cert',
            default_headers=self._get_headers,
            error_handler=HTTPErrorHandler(MDBSecretsError),
            verify=config.cert_api.ca_path,
        )
        self.ca_name = self.config.cert_api.ca_name
        self.cert_type = self.config.cert_api.cert_type

    def _get_headers(self):
        if self.iam_jwt is not None:
            jwt_token = self.iam_jwt.get_token()
            return {
                'Authorization': f'Bearer {jwt_token}',
            }
        return {
            'Authorization': f'OAuth {self.config.cert_api.token}',
        }

    @retry.on_exception(MDBSecretsError, factor=10, max_wait=60, max_tries=6)
    @tracing.trace('MDBSecrets Issue')
    def issue(
        self,
        hostname,
        alt_names,
        force_tls_certs=False,
    ) -> IssueResult:
        """
        Issue new cert with MDB Secrets (or get existing)
        """
        tracing.set_tag('tls.cert.fqdn', hostname)
        tracing.set_tag('tls.cert.alt_names', alt_names)

        params = {
            'ca': self.ca_name,
            'hostname': hostname,
            'alt_names': ",".join(alt_names),
            'type': self.cert_type,
        }

        if force_tls_certs:
            params['force'] = 'true'

        res = self._make_request('', 'put', params=params)
        res_ekey = res['key']
        ekey = {'encryption_version': res_ekey['version'], 'data': res_ekey['data']}
        return IssueResult(ekey, res['cert'], res['expiration'])

    @retry.on_exception(MDBSecretsError, factor=10, max_wait=60, max_tries=6)
    @tracing.trace('MDBSecrets Revoke')
    def revoke(self, hostname) -> None:
        """
        Revoke all issued certs for hostname
        """
        tracing.set_tag('tls.cert.fqdn', hostname)

        self.add_change(Change(f'tls-cert.{hostname}', 'revoke'))
        self._make_request('', 'delete', params={'hostname': hostname}, expect=[200, 404])
