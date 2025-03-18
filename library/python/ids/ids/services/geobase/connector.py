# -*- coding: utf-8 -*-
from __future__ import unicode_literals

from ids.connector import HttpConnector
from ids.connector import plugins_lib


class GeobaseConnector(HttpConnector):
    service_name = 'GEOBASE'

    url_patterns = {
        'parents': '/v1/parents',
        'region': '/v1/{func}',
    }

    plugins = (
        plugins_lib.JsonResponse,
    )
