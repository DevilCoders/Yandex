"""
Module for resource manager Api
"""

from abc import ABC, abstractmethod

import requests
from flask import current_app

from dbaas_common import retry, tracing

from cloud.mdb.internal.python.logs import MdbLoggerAdapter
from cloud.mdb.internal.python import grpcutil
from cloud.mdb.internal.python.compute.folders import FoldersClient, FoldersClientConfig

from ..utils.request_context import get_x_request_id
from ..utils import iam_jwt

from .logs import get_logger


# pylint: disable=too-few-public-methods
class ResourceManager(ABC):
    """
    Abstract Resource manager provider
    """

    @abstractmethod
    def service_account_roles(self, folder_id, service_account_id):
        """
        Returns a list of roles that the service account has within folder
        """


class ResourceManagerError(Exception):
    """
    Base resource manager error
    """


class ResourceManagerApi(ResourceManager):
    """
    Resource manager API provider
    """

    def __init__(self, config):
        rm_config = config['RESOURCE_MANAGER_CONFIG']
        self.api_url = rm_config['url']
        self.ca_path = rm_config['ca_path']
        self.iam_jwt = iam_jwt.get_provider()

    @retry.on_exception((ResourceManagerError, requests.RequestException), max_tries=3)
    def _make_request(self, path, params=None, expected_status_codes=None):
        """
        Make request to API with params
        """
        if expected_status_codes is None:
            expected_status_codes = [200]
        headers = {
            'Authorization': f'Bearer {self.iam_jwt.get_iam_token()}',
            'X-Request-Id': get_x_request_id(),
        }
        kwargs = {
            'url': self.api_url + '/' + path.lstrip('/'),
            'headers': headers,
            'timeout': (1, 10),
            'verify': self.ca_path,
        }
        if params:
            kwargs['params'] = params
        res = requests.get(**kwargs)
        if res.status_code not in expected_status_codes:
            raise ResourceManagerError(f'Unexpected result: {res.status_code} {res.text}')
        return res.json()

    @tracing.trace('ResourceManager ServiceAccountRoles')
    def service_account_roles(self, folder_id, service_account_id):
        tracing.set_tag('cloud.folder.id', folder_id)
        tracing.set_tag('auth.service_account_id', service_account_id)

        roles = set()
        params = {'pageSize': 1000}
        url = f'folders/{folder_id}:listAccessBindings'
        while True:
            data = self._make_request(url, params=params)
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


class ResourceManagerGRPC(ResourceManager):
    def __init__(self, config):
        rm_config = config['RESOURCE_MANAGER_CONFIG_GRPC']
        self.logger = MdbLoggerAdapter(
            get_logger(),
            extra={
                'request_id': get_x_request_id(),
            },
        )
        self.iam_jwt = iam_jwt.get_provider()

        self._folders_client_config = FoldersClientConfig(
            transport=grpcutil.Config(
                url=rm_config['url'],
                cert_file=rm_config['cert_file'],
            )
        )

    @tracing.trace('ResourceManager ServiceAccountRoles')
    def service_account_roles(self, folder_id, service_account_id):
        tracing.set_tag('cloud.folder.id', folder_id)
        tracing.set_tag('auth.service_account_id', service_account_id)

        roles = set()
        with FoldersClient(
            config=self._folders_client_config,
            logger=self.logger,
            token_getter=self.iam_jwt.get_iam_token,
            error_handlers={},
        ) as client:
            for binding in client.list_access_bindings(folder_id):
                if binding.subject.type != 'serviceAccount':
                    continue
                if binding.subject.id != service_account_id:
                    continue
                roles.add(binding.role_id)
        return roles


def get_manager() -> ResourceManager:
    """
    Get resource manager according to config and flags
    """
    return current_app.config['RESOURCE_MANAGER'](current_app.config)


def service_account_roles(folder_id, service_account_id):
    """
    Check if service account have role in folder
    """
    manager = get_manager()
    return manager.service_account_roles(folder_id, service_account_id)
