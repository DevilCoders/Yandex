# coding: utf-8

import os
import json
from codecs import open

import requests_mock

from .url import api_url

import yatest.common as yc


def backend_response(file):
    prefix = 'library/python/startrek_python_client/tests_int/common/backend_responses'
    path = yc.source_path(os.path.join(prefix, file))
    with open(path, encoding='utf-8') as f:
        return json.load(f)


def base_mock():
    m = requests_mock.mock()
    m.get(api_url('/fields/'), json=backend_response('fields.json'))
    return m
