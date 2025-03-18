# coding: utf-8

from __future__ import unicode_literals

from ids.connector import HttpConnector


class WikiFormatterConnector(HttpConnector):
    service_name = 'FORMATTER'

    url_patterns = {
        'html': '/v{version}/html?cfg={cfg}',
        'bemjson': '/v{version}/bemjson?cfg={cfg}',
        'diff': '/v{version}/diff?json={json}',
    }
