# coding: utf-8

from __future__ import unicode_literals

from ids.connector import HttpConnector
from ids.connector import plugins_lib


class GoalsConnector(HttpConnector):

    service_name = 'GOALS'

    url_patterns = {
        'goals': '/api/v1/goals',
    }

    plugins = (
        plugins_lib.OAuth,
        plugins_lib.JsonResponse,
    )
