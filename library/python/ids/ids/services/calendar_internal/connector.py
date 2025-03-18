# coding: utf-8
from __future__ import unicode_literals

from ids.connector import HttpConnector
from ids.connector import plugins_lib


class CalendarInternalError(Exception):
    def __init__(self, name, message):
        super(CalendarInternalError, self).__init__(message)
        self.name = name
        self.message = message


class CalendarInternalConnector(HttpConnector):
    """
    Внутренний API календаря

    https://wiki.yandex-team.ru/Calendar/api/new-web/
    """
    service_name = 'CALENDAR_INTERNAL'

    default_connect_timeout = 5

    url_prefix = '/internal/'
    url_patterns = {
        'schedule': 'get-resources-schedule',
        'office': 'get-offices',
        'holidays': 'get-holidays',
        'event': '{method}-event',
    }

    plugins = [
        plugins_lib.get_disjunctive_plugin_chain([
            plugins_lib.OAuth,
            plugins_lib.TVM2UserTicket,
            plugins_lib.TVM2ServiceTicket,
        ]),
        plugins_lib.JsonResponse,
    ]

    def build_url(self, resource=None, url_vars=None):
        # Обратная совместимость
        if resource == 'event':
            url_vars = url_vars or {}
            url_vars.setdefault('method', 'get')
        return super(CalendarInternalConnector, self).build_url(resource, url_vars)
