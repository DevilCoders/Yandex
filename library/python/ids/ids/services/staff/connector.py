# coding: utf-8
from __future__ import unicode_literals

from ids.connector import HttpConnector
from ids.connector import plugins_lib


class StaffConnector(HttpConnector):

    MAX_LENGTH_OF_QUERY_PARAMS = 700

    service_name = 'STAFF'

    url_patterns = {
        'equipment': '/v3/equipment',
        'group': '/v3/groups',
        'groupmembership': '/v3/groupmembership',
        'occupation': '/v3/occupations',
        'office': '/v3/offices',
        'organization': '/v3/organizations',
        'person': '/v3/persons',
        'position': '/v3/positions',
        'geography': '/v3/geographies',
        'room': '/v3/rooms',
        'table': '/v3/tables',
    }

    plugins = [
        plugins_lib.get_disjunctive_plugin_chain([
            plugins_lib.OAuth,
            plugins_lib.TVM2UserTicket,
            plugins_lib.TVM2ServiceTicket,
        ]),
        plugins_lib.JsonResponse,
    ]

    def execute_request(self, method='get', resource=None, url_vars=None, **params):
        if method.lower() == 'get':
            query_params = '&'.join('%s=%s' % key_val for key_val in params.get('params', {}).items())
            if len(query_params) > self.MAX_LENGTH_OF_QUERY_PARAMS:
                # если длина строки с параметрами слишком большая, отправляем POST запрос вместо GET
                method = u'POST'
                params['data'] = params.pop('params')

        return super(StaffConnector, self).execute_request(method, resource, url_vars, **params)
