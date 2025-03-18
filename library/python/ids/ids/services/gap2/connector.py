# coding: utf-8

from __future__ import unicode_literals

from ids.connector import HttpConnector
from ids.connector import plugins_lib


class Gap2Connector(HttpConnector):
    """
    https://wiki.yandex-team.ru/staff/pool/gap2/api/
    """
    service_name = 'GAP2'

    url_prefix = '/gap-api/api'
    url_patterns = {
        'gaps_detail': '/gap_find/{gap_id}/',
        'gaps': '/gaps_find/',
    }

    plugins = [
        plugins_lib.OAuth,
        plugins_lib.JsonResponse,
    ]
