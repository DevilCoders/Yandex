# coding: utf-8

from __future__ import unicode_literals

from ids.connector import http
from ids.connector import plugins_lib

from django_ids import settings
from django_ids import auth


class HttpConnector(http.HttpConnector):

    available_auth_types = (
        'oauth',
        'session_id',
    )

    plugins = (
        auth.MultiAuthPlugin,
        plugins_lib.JsonResponse,
    )

    def __init__(self, *args, **kwargs):
        if 'user_agent' not in kwargs:
            kwargs['user_agent'] = settings.IDS_DEFAULT_USER_AGENT
        super(HttpConnector, self).__init__(*args, **kwargs)
