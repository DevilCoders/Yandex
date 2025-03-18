# coding: utf-8
from __future__ import unicode_literals

from ids.connector import HttpConnector
from ids.connector import plugins_lib


class OrangeConnector(HttpConnector):
    service_name = 'ORANGE'

    url_patterns = {
        'ping': '/v1/system/ping',
        'notifications_push': '/v1/notifications/push',
        'notifications_push_callback': '/v1/notifications/push-callback',
        'notifications_delete': '/v1/notifications/{id}',
        'notifications_delete_user': '/v1/notifications/{id}/user/{uid}',
    }

    plugins = [
        plugins_lib.OAuth,
    ]

