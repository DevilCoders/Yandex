"""
Module for resource manager Api
"""
from dbaas_common import tracing

from ..exceptions import ExposedException
from .http import HTTPClient, HTTPErrorHandler
from .iam_jwt import IamJwt


class ResourceManagerApiError(ExposedException):
    """
    Base resource manager error
    """


class ResourceManagerApi(HTTPClient):
    """
    Resource manager provider. Uses per instance cache without invalidation. Should not be reused
    """

    def __init__(self, config, task, queue):
        super().__init__(config, task, queue)
        self.token_service = IamJwt(config, task, queue)
        self._init_session(
            self.config.resource_manager.url,
            default_headers=self._headers,
            error_handler=HTTPErrorHandler(ResourceManagerApiError),
            verify=self.config.resource_manager.ca_path,
        )
        self._folder_role_cache = dict()

    def _headers(self):
        token = self.token_service.get_token()
        return {
            'Authorization': f'Bearer {token}',
        }

    def _get_cached_folder_roles(self, folder_id, service_account_id):
        key = folder_id, service_account_id
        return self._folder_role_cache.get(key)

    def _set_cached_folder_roles(self, folder_id, service_account_id, roles):
        key = folder_id, service_account_id
        self._folder_role_cache[key] = roles

    def _get_folder_roles(self, folder_id, service_account_id):
        params = {'pageSize': 1000}
        path = f'folders/{folder_id}:listAccessBindings'
        roles = set()
        while True:
            data = self._make_request(path, params=params)
            for binding in data.get('accessBindings', []):
                subject = binding.get('subject', {})
                if subject.get('type') != 'serviceAccount':
                    continue
                if subject.get('id') != service_account_id:
                    continue
                roles.add(binding.get('roleId'))
            token = data.get('nextPageToken')
            if not token:
                break
            params['pageToken'] = token
        return roles

    @tracing.trace('Resource Manager List Folder Roles')
    def list_folder_roles(self, folder_id, service_account_id):
        """
        Check if service account have role in folder
        """
        tracing.set_tag('cloud.folder.id', folder_id)
        tracing.set_tag('account.service.id', service_account_id)

        roles = self._get_cached_folder_roles(folder_id, service_account_id)
        if roles is not None:
            return roles
        roles = self._get_folder_roles(folder_id, service_account_id)
        self._set_cached_folder_roles(folder_id, service_account_id, roles)
        return roles
