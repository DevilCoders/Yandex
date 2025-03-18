# coding: utf-8
from __future__ import unicode_literals

from ids.connector import HttpConnector
from ids.connector import plugins_lib


class ABCConnector(HttpConnector):
    service_name = 'ABC'

    default_url_patterns = {
        'service': '/api/v{api_version}/services/',
        'service_contacts': '/api/v{api_version}/services/contacts/',
        'service_members': '/api/v{api_version}/services/members/',
        'role': '/api/v{api_version}/roles/',
        'resource': '/api/v{api_version}/resources/',
    }

    plugins = [
        plugins_lib.get_disjunctive_plugin_chain([
            plugins_lib.OAuth,
            plugins_lib.TVM2UserTicket,
            plugins_lib.TVM2ServiceTicket,
        ]),
        plugins_lib.JsonResponse,
    ]

    def __init__(self, api_version=3, *args, **kwargs):
        for resource, path in self.default_url_patterns.items():
            if api_version < 4 and resource == 'service_members':
                self.url_patterns[resource] = path.format(api_version=4)
            else:
                self.url_patterns[resource] = path.format(api_version=api_version)

        super(ABCConnector, self).__init__(*args, **kwargs)
