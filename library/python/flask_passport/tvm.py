# -*- coding: utf-8 -*-
from __future__ import absolute_import, division, print_function, unicode_literals

import functools
import time
import six

import requests
import ticket_parser2 as tp2
from ticket_parser2.low_level import ServiceContext


def cache(seconds=600):
    store = {}

    def decorator(fn):
        @functools.wraps(fn)
        def wrapper(*args, **kwargs):
            key = (tuple(args), tuple(sorted(kwargs.items())))
            now = int(time.time())
            if key in store:
                timestamp, value = store[key]
                if now < seconds + timestamp:
                    return value
            value = fn(*args, **kwargs)
            store[key] = (now, value)
            return value

        return wrapper

    return decorator


def to_native_str(b):
    if six.PY3 and hasattr(b, 'decode'):
        return b.decode()
    return b


class Tvm(object):
    def __init__(self, src_id, secret, blackbox_tvm_id=223, url="tvm-api.yandex.net"):
        self.src_id = src_id
        self.__secret = secret
        self.blackbox_tvm_id = blackbox_tvm_id
        self.tvm_api_url = url

    @cache(seconds=86400)
    def get_service_context(self):
        tvm_keys = requests.get(
            'https://{tvm_api_url}/2/keys?lib_version={version}'.format(
                tvm_api_url=self.tvm_api_url,
                version=tp2.__version__,
            )).content
        return ServiceContext(self.src_id, self.__secret, to_native_str(tvm_keys))

    @cache(seconds=3600)
    def get_service_ticket(self):
        dst = str(self.blackbox_tvm_id)
        ts = int(time.time())
        context = self.get_service_context()
        ticket_response = requests.post(
            'https://{}/2/ticket/'.format(self.tvm_api_url),
            data={
                'grant_type': 'client_credentials',
                'src': self.src_id,
                'dst': dst,
                'ts': ts,
                'sign': context.sign(ts, dst)
            },
        ).json()
        ticket = ticket_response[dst]['ticket']
        return ticket
