# -*- coding: utf-8 -*-
from __future__ import absolute_import

import os

import math
import six

os.environ['DJANGO_SETTINGS_MODULE'] = 'ylog.tests.django_settings'

try:
    from django.test import client
    DJANGO_AVAILABLE = True
except ImportError:
    DJANGO_AVAILABLE = False

from nose.tools import eq_
from ..format import format_post, POST_TRUNCATE_SIZE
from .utils import skip_if_not


@skip_if_not(DJANGO_AVAILABLE)
def test_format_django_request():
    factory = client.RequestFactory()

    # обычный multipart/form-data
    request = factory.post('/', {'a': u'блах'})
    if six.PY2:
        expected = """<QueryDict: {u'a': [u'\\u0431\\u043b\\u0430\\u0445']}>"""
    else:
        expected = """<QueryDict: {'a': ['блах']}>"""
    eq_(expected, format_post(request))

    # небольшой объем данных в JSON
    request = factory.post('/', '{"a": "блах"}', content_type='application/json')
    eq_(
        b'{"a": "\xd0\xb1\xd0\xbb\xd0\xb0\xd1\x85"}',
        format_post(request)
    )

    # большой объем данных в plain/text

    # Формируем буфер с данными, превышающий тот лимит, который
    # выводит format_post.
    block = u'блах'.encode('utf-8')
    num_blocks = int(math.floor(POST_TRUNCATE_SIZE / len(block))) + 10
    data = block * num_blocks

    request = factory.post('/', data , content_type='plain/text')
    eq_(
        data[:POST_TRUNCATE_SIZE] + b'...',
        format_post(request)
    )

