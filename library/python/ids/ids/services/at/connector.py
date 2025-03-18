# coding: utf-8
from __future__ import unicode_literals

import logging
import yenv

from ids.connector import HttpConnector
from ids.connector import plugins_lib


logger = logging.getLogger(__name__)


class Connector(HttpConnector):
    service_name = 'AT'

    url_prefix = '/api/yaru'

    url_patterns = {
        'person_info': '/person/{uid}/',
        'person_posts': '/person/{uid}/post/',
        'person_typed_posts': '/person/{uid}/post/{post_type}/',
        'person_post': '/person/{uid}/post/{post_no}/',
        'person_comments': '/person/{uid}/post/{post_no}/comment/',

        'person_clubs': '/person/{uid}/club/owner/',
        'person_friends': '/person/{uid}/friend/',

        'club_info': '/club/{uid}/',
        'club_posts': '/club/{uid}/post/',
        'club_typed_posts': '/club/{uid}/post/{post_type}/',
        'club_post': '/club/{uid}/post/{post_no}/',
        'club_comments': '/club/{uid}/post/{post_no}/comment/',
    }

    plugins = [
        plugins_lib.get_disjunctive_plugin_chain([
            plugins_lib.TVM2UserTicket,
            plugins_lib.OAuth,
        ]),
        plugins_lib.JsonResponse,
    ]

    def prepare_params(self, params):
        query = params.get('params', {})
        query['format'] = 'json'
        params['params'] = query
