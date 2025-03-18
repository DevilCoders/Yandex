"""
    Racktables NEW api
"""

import requests


class Racktables(object):
    API_PRODUCTION = 'https://racktables.yandex-team.ru/export'
    API_TESTING = ''

    def __init__(self, token):
        self.token = token
        self.api = Racktables.API_PRODUCTION

    def request(self, api_path, query=None, data=None, headers=None, method='GET'):
        full_headers = {'Authorization': 'OAuth {}'.format(self.token)}
        full_headers.update(headers or {})

        method_url = '{}{}'.format(self.api, api_path)
        for key, value in (query or {}).items():
            sep = '?' if '?' not in method_url else '&'
            method_url = '{}{}{}={}'.format(method_url, sep, key, value)

        if method.upper() == 'GET':
            return requests.get(method_url, data=data, headers=full_headers, timeout=20).json()
        elif method.upper() == 'POST':
            response = requests.post(method_url, data=data, headers=full_headers)
            if response.status_code != 200:
                raise RuntimeError(response.text)
            return response.json()
        raise ValueError('Unsuported HTTP method {}.'.format(method))

    def list_macros(self):
        return self.request('/project-id-networks.php', query={'op': 'list_macros'})

    def show_macro(self, macro_name):
        return self.request('/project-id-networks.php', query={'op': 'show_macro', 'name': macro_name})

    def list_ranges(self):
        return self.request('/project-id-networks.php', query={'op': 'list_ranges'})

    def create_macro(self, macro_name, ownsers=None, parant_macro_name='', desc='', secured=0):
        query = {'op': 'create_macro', 'name': macro_name}
        if ownsers:
            query.udpate({'owners': ','.join(ownsers)})
        if parant_macro_name:
            query.update({'parent': parant_macro_name})
        if desc:
            query.update({'description': desc})
        if secured:
            query.update({'secured': secured})
        return self.request('/project-id-networks.php', query=query, method='POST')

    def edit_macro(self, macro_name, owners=None, parant_macro_name='', desc=''):
        query = {'op': 'edit_macro', 'name': macro_name}
        if owners:
            query.update({'owners': ','.join(owners)})
        if parant_macro_name:
            query.update({'parent': parant_macro_name})
        if desc:
            query.update({'description': desc})
        return self.request('/project-id-networks.php', query=query, method='POST')

    def create_network(self, macro_or_range_name, project_id, scope=None, desc=''):
        query = {'op': 'create_network', 'macro_name': macro_or_range_name}
        if project_id:
            query.update({'project_id': '{:x}'.format(project_id)})
        if scope:
            query.update({'scope': scope})
        if desc:
            query.update({'description': desc})
        return self.request('/project-id-networks.php', query=query, method='POST')


    def delete_network(self, macro_or_range_name, project_id, scope=None):
        query = {'op': 'delete_network', 'project_id': '{:x}'.format(project_id)}
        if macro_or_range_name:
            query.update({'macro_name': macro_or_range_name})
        if scope:
            query.update({'scope': scope})
        return self.request('/project-id-networks.php', query=query, method='POST')

    def move_network(self, macro_or_range_name, project_id, scope=None):
        query = {'op': 'move_network', 'macro_name': macro_or_range_name, 'project_id': '{:x}'.format(project_id)}
        if scope:
            query.update({'scope': scope})
        return self.request('/project-id-networks.php', query=query, method='POST')

    def macros_owners(self, macro_name=None):
        response = self.request('/network-macros-owners.php')
        if macro_name:
            return response[macro_name]
        return response

