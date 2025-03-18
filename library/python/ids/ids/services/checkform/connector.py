# -*- coding: utf-8 -*-
from __future__ import unicode_literals

from ids.connector import HttpConnector


class CheckFormConnector(HttpConnector):
    service_name = 'CHECKFORM'

    url_patterns = {
        'check': '/check',
    }
