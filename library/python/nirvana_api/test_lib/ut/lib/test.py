#!/usr/bin/env python
# -*- coding: utf-8 -*-
import pytest


from library.python.nirvana_api.test_lib import NirvanaApiMock


class ApiHolder(object):
    def __init__(self):
        self._out = []
        self._api = NirvanaApiMock(output=self)

    def write(self, some_string):
        self._out.append(some_string)

    @property
    def api(self):
        return self._api

    @property
    def out(self):
        return self._out


class ReadObj(object):
    def __init__(self, content):
        self._content = content

    def read(self):
        return self._content


@pytest.fixture
def api_holder():
    return ApiHolder()


def test_request(api_holder):
    api_holder.api.get_username()
    assert 'getOrCreateUser' in ''.join(api_holder.out)


def test_batch_request(api_holder):
    with api_holder.api.batch():
        api_holder.api.get_username()
    assert 'getOrCreateUser' in ''.join(api_holder.out)


def test_multipart_request(api_holder):
    api_holder.api.upload_data_multipart(data_id='my_id', upload_parameters=dict(), file=ReadObj('hello'))
    assert 'uploadData' in ''.join(api_holder.out)
