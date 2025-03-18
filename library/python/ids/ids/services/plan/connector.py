# coding: utf-8
from __future__ import unicode_literals

from ids.connector import HttpConnector
from ids.connector import plugins_lib


class PlanConnectorDeprecated(HttpConnector):
    service_name = 'PLAN_API'

    url_patterns = {
        'service': '/yandex-intranet-api-plan-api-v1/services',
        'servicedetails': '/yandex-intranet-api-plan-api-v1/service',
        'project': '/yandex-intranet-api-plan-api-v1/projects',
        'projectdetails': '/yandex-intranet-api-plan-api-v1/project',
    }

    def search(self, resource, **params):
        page = pages = 1
        while page <= pages:
            params.update({'_page': page})
            response = self.get(resource, params=params)

            for item in response['result']:
                yield item

            pages = response['pages']
            page = response['page'] + 1

    def get_one(self, resource, **params):
        return self.get(resource, params=params)

    def handle_response(self, response):
        return response.json()


class PlanConnector(HttpConnector):
    service_name = 'PLAN'

    url_patterns = {
        'services': '/v2/services',
        'projects': '/v2/projects',
        'roles': '/v2/roles',
        'role_scopes': '/v2/role_scopes',
        'service_bundles': '/v2/service_bundles',
        'directions': '/v2/directions',
    }

    plugins = [
        plugins_lib.OAuth,
        plugins_lib.JsonResponse,
    ]
