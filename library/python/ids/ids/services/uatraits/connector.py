# -*- coding: utf-8 -*-
from __future__ import unicode_literals

from ids.connector import HttpConnector


class UatraitsConnector(HttpConnector):
    service_name = 'UATRAITS'

    url_patterns = {
        'detect': '/v0/detect',
    }

    bad_browser = {
        'BrowserEngine': 'Unknown',
        'BrowserName': 'Unknown',
        'OSFamily': 'Unknown',
        'isBrowser': False,
        'isMobile': False,
    }

    def handle_response(self, response):
        if response.status_code == 200:
            return response.json() or dict(self.bad_browser)
        return dict(self.bad_browser)

    def handle_bad_response(self, response):
        pass
