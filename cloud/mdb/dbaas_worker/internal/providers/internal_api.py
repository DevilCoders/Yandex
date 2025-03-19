"""
MDB Internal API interaction module
"""
from dbaas_common import tracing

from .http import HTTPClient, HTTPErrorHandler
from ..exceptions import ExposedException


class InternalApiError(ExposedException):
    """
    Base MDB Internal API error
    """


class InternalApi(HTTPClient):
    """
    MDB Internal API provider
    """

    def __init__(self, config, task, queue, api_url: str = None):
        super().__init__(config, task, queue)
        headers = {
            'Access-Id': self.config.internal_api.access_id,
            'Access-Secret': self.config.internal_api.access_secret,
        }
        self._init_session(
            api_url or self.config.internal_api.url,
            default_headers=headers,
            error_handler=HTTPErrorHandler(InternalApiError),
            verify=self.config.internal_api.ca_path,
        )

    @tracing.trace('HTTP Internal API Get Config Unmanaged')
    def get_fqdn_pillar_cfg_unmanaged(self, fqdn, rev=None):
        """
        Get pillar unmanaged config for fqdn
        """
        tracing.set_tag('cluster.host.fqdn', fqdn)

        params = {}
        if rev is not None:
            tracing.set_tag('cluster.rev', rev)
            params['rev'] = rev

        return self._make_request('api/v1.0/config_unmanaged/{fqdn}'.format(fqdn=fqdn), params=params)

    @tracing.trace('HTTP Internal API Get Backups')
    def get_backups(self, cluster_type, cluster_id):
        tracing.set_tag('cluster.id', cluster_id)
        return self._make_request(
            f'mdb/{cluster_type}/1.0/clusters/{cluster_id}/backups',
            params={'pageSize': 1000},
        )
