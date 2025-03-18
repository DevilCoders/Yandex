#!/skynet/python/bin/python
# coding: utf8
from aux_http_api import HttpApi
import sys


class Walle(HttpApi):
    API_URL_PRODUCTION = 'https://api.wall-e.yandex-team.ru'

    def __init__(self, oauth_token):
        super(Walle, self).__init__(self.API_URL_PRODUCTION, oauth_token)

    def projects(self):
        return self._get('/v1/projects')['result']

    def hosts(self, fields=None):
        fields = fields or 'name,project,status,state,location.switch'

        host_info = []
        limit = 10000
        cursor = 0
        while True:
            query = {'fields': fields, 'limit': limit, 'cursor': cursor}
            api_result = self._get('/v1/hosts', query=query)
            result = api_result['result']
            cursor = api_result.get('next_cursor', None)

            assert result
            host_info += result

            if cursor is None:
                break

        return host_info

    def hosts_info(self):
        projects_info = {}
        for project_item in self.projects():
            projects_info[project_item['id']] = project_item

        hosts_info = {}
        corrupted_items = list()
        for host_item in self.hosts():
            if 'name' not in host_item:
                corrupted_items.append(host_item)
                continue
            hostname = host_item['name']
            project_name = host_item['project']

            hosts_info[hostname] = host_item
            hosts_info[hostname]['tags'] = projects_info[project_name].get('tags', [])

        if corrupted_items:
            print >> sys.stderr, "corrupted_items count {}".format(len(corrupted_items))
            print >> sys.stderr, "corrupted items repr = {}".format(repr(corrupted_items))

        assert len(corrupted_items) <= 10

        return hosts_info


if __name__ == '__main__':
    import os
    import sys
    import json

    sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))

    import gencfg  # noqa
    import config

    walle = Walle(config.get_default_oauth())
    print(json.dumps(walle.hosts_info(), indent=4, sort_keys=True))
