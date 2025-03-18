# coding: utf-8
from __future__ import division, absolute_import, print_function, unicode_literals
import requests.exceptions
from statface_client.report.api import (
    UploadChecker,
    JUST_ONE_CHECK,
    DO_NOTHING,
    ENDLESS_WAIT_FOR_FINISH,
)
from statface_client import (
    StatfaceHttpResponseError,
    StatfaceClientDataUploadError,
)
import pytest


class VerySpecialError(Exception):
    pass


class YetAnotherCustomError(Exception):
    pass


class DummyApi(object):
    def check_data_upload_status_verbose(self, *args, **kwargs):
        raise VerySpecialError

    def fetch_data_upload_status(self, *args, **kwargs):
        return self.check_data_upload_status_verbose()['status']


class DummyApiWithoutProblem(DummyApi):
    def check_data_upload_status_verbose(self, *args, **kwargs):
        return dict(status='success', comment='fake')


class DummyApiWithNetworkProblem(DummyApi):
    def check_data_upload_status_verbose(self, *args, **kwargs):
        raise requests.exceptions.HTTPError


class DummyResponse(object):

    def __init__(self, status_code, reason=''):
        self.status_code = status_code
        self.reason = reason
        self.elapsed = 7.1
        self.text = 'zuzuzu'


def test_upload_checher():
    with UploadChecker(DummyApi(), ENDLESS_WAIT_FOR_FINISH, 'some_id'):
        assert True

    with pytest.raises(requests.exceptions.ConnectionError):
        with UploadChecker(DummyApi(), ENDLESS_WAIT_FOR_FINISH, 'some_id'):
            raise requests.exceptions.ConnectionError

    with pytest.raises(VerySpecialError):
        with UploadChecker(DummyApi(), JUST_ONE_CHECK, 'some_id'):
            raise requests.exceptions.ReadTimeout

    with UploadChecker(DummyApiWithoutProblem(), ENDLESS_WAIT_FOR_FINISH, 'some_id'):
        raise requests.exceptions.Timeout

    with pytest.raises(requests.exceptions.ReadTimeout):
        with UploadChecker(DummyApiWithNetworkProblem(),
                           ENDLESS_WAIT_FOR_FINISH, 'some_id'):
            raise requests.exceptions.ReadTimeout

    with pytest.raises(VerySpecialError):
        with UploadChecker(DummyApi(), ENDLESS_WAIT_FOR_FINISH, 'some_id'):
            raise requests.exceptions.Timeout

    with UploadChecker(DummyApi(), DO_NOTHING, 'some_id'):
        raise requests.exceptions.Timeout

    with pytest.raises(StatfaceHttpResponseError):
        with UploadChecker(DummyApi(), DO_NOTHING, 'some_id'):
            raise StatfaceHttpResponseError(DummyResponse(500), None)

    with pytest.raises(StatfaceHttpResponseError):
        with UploadChecker(DummyApi(), DO_NOTHING, 'some_id'):
            raise StatfaceHttpResponseError(DummyResponse(403), None)

    with pytest.raises(YetAnotherCustomError):
        with UploadChecker(DummyApi(), JUST_ONE_CHECK, 'some_id'):
            raise YetAnotherCustomError
