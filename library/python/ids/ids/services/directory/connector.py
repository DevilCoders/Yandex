# coding: utf-8
from __future__ import unicode_literals

from ids.connector import HttpConnector
from ids.connector import plugins_lib


class DirectoryTokenPlugin(plugins_lib.SetRequestHeadersPlugin):
    required_params = ['token']

    def get_headers(self):
        return {'Authorization': 'Token ' + self.connector.token}


class DirectoryConnector(HttpConnector):
    # Документация АПИ – https://api.directory.ws.yandex.ru/docs/index.html

    service_name = 'DIRECTORY'

    default_url_patterns = {
        'user': {'path': '{api_version}/users/', 'default_version': None},
        'user_v7': {'path': '/v7/users/'},
        'single_user': {'path': '{api_version}/users/{{uid}}/', 'default_version': None},
        'single_cloud_user': {'path': '{api_version}/users/cloud/{{uid}}/', 'default_version': None},
        'group': {'path': '{api_version}/groups/', 'default_version': None},
        'organization': {'path': '{api_version}/organizations/', 'default_version': 2},
        'single_organization': {'path': '{api_version}/organizations/{{id}}/', 'default_version': 2},
        'department': {'path': '{api_version}/departments/', 'default_version': 2},
        'who_is': {'path': '{api_version}/who-is/',  'default_version': 9},
    }

    plugins = (
        plugins_lib.get_disjunctive_plugin_chain(
            [
                DirectoryTokenPlugin, plugins_lib.Tvm,
                plugins_lib.TVM2UserTicket, plugins_lib.TVM2ServiceTicket,
            ]
        ),
        plugins_lib.JsonResponse,
    )

    def __init__(self, api_version=None, *args, **kwargs):
        for resource, path_data in self.default_url_patterns.items():
            api_version_for_path = api_version or path_data.get('default_version')
            if api_version_for_path:
                api_version_for_path = '/v{}'.format(api_version_for_path)
            self.url_patterns[resource] = path_data['path'].format(api_version=api_version_for_path or '')

        super(DirectoryConnector, self).__init__(*args, **kwargs)

    def prepare_params(self, params):
        super(DirectoryConnector, self).prepare_params(params)

        lookup = params.get('params', None)
        if lookup is None:
            params['params'] = lookup = {}

        headers = params.get('headers', None)
        if headers is None:
            params['headers'] = headers = {}

        for param_name, header_name in (
            ('x_org_id', 'X-Org-ID'),
            ('x_uid', 'X-UID'),
            ('x_user_ip', 'X-USER-IP'),
            ('x_cloud_uid', 'X-CLOUD-UID')
        ):
            value = lookup.pop(param_name, None)
            if value is not None:
                headers[header_name] = str(value)
