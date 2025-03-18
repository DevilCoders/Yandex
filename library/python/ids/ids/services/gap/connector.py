# coding: utf-8

from __future__ import unicode_literals

from ids.connector import HttpConnector


class GapConnector(HttpConnector):
    service_name = 'GAP'
    required_params = [
        'token',
    ]

    url_patterns = {
        'gap_list': '/api/gap/?period_from={period_from}&period_to={period_to}'
                    '&login_list={login_list}',
    }

    def get_query_params(self, resource, url_vars):
        return {
            'token': self.token
        }
