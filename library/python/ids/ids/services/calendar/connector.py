# coding: utf-8
from __future__ import unicode_literals

from ids.connector import HttpConnector


class CalendarConnector(HttpConnector):
    service_name = 'CALENDAR'

    url_patterns = {
        'holidays': '/export/holidays.xml',
    }

    def handle_response(self, response):
        return response.text
