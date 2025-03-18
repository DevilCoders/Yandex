# coding: utf-8
from __future__ import unicode_literals

import logging

from ids.connector import HttpConnector
from ids.connector import plugins_lib

logger = logging.getLogger(__name__)


class InflectorConnector(HttpConnector):
    service_name = 'INFLECTOR'

    url_patterns = {
        'title': '/wizard?wizclient={wizclient}&action=inflect&text='
                 'persn{{{first_name}}}'
                 'famn{{{last_name}}}'
                 'patrn{{{middle_name}}}'
                 'gender{{{gender}}};fio=1',
        'word': '/wizard?wizclient={wizclient}&action=inflect&text={word};fio={fio}',
        'title_no_order': '/wizard?wizclient={wizclient}&action=inflect&text={title};fio=1'
    }

    plugins = [
        plugins_lib.JsonResponse
    ]
