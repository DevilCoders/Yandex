# coding: utf-8
from __future__ import unicode_literals

from ids.connector import HttpConnector
from ids.connector import plugins_lib


class IntrasearchConnector(HttpConnector):

    service_name = 'INTRASEARCH'

    url_patterns = {
        'suggest': '/suggest/',
    }

    plugins = [
        plugins_lib.get_disjunctive_plugin_chain(
            [plugins_lib.OAuth, plugins_lib.TVM2UserTicket]
        ),
        plugins_lib.JsonResponse,
    ]
