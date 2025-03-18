# coding: utf-8

from __future__ import unicode_literals

from unittest import TestCase
import vcr
import os
import pytest
from mock import Mock, patch
import blackbox
from django.test.utils import override_settings
from django.test import RequestFactory
from django.conf import settings
from django.contrib.auth import authenticate

from tvm2 import TVM2

from tvmauth import BlackboxTvmId as BlackboxClientId, TvmClientStatus
from tvmauth.mock import TvmClientPatcher, MockedTvmClient

try:
    import yatest
    path = yatest.common.source_path('library/python/python-django-yauth/vcr_cassettes')
except ImportError:
    path = os.path.abspath(os.path.join(os.curdir, 'vcr_cassettes'))

test_vcr = vcr.VCR(
    cassette_library_dir=path,
)

TEST_YANDEX_EMAIL = '%s@yandex.ru' % settings.TEST_YANDEX_LOGIN


def get_tvm2_client():
    TVM2._instance = None
    with TvmClientPatcher(MockedTvmClient(self_tvm_id=int(TVM_SETTINGS['YAUTH_TVM2_CLIENT_ID']))):
        return TVM2(
            client_id=TVM_SETTINGS['YAUTH_TVM2_CLIENT_ID'],
            blackbox_client=TVM_SETTINGS['YAUTH_TVM2_BLACKBOX_CLIENT'],
            secret=TVM_SETTINGS['YAUTH_TVM2_SECRET'],
        )


class MockBlackbox(object):
    url = 'mock-blackbox'

    def __init__(self, **params):
        self.params = params

        defaults = dict(valid=True, uid=settings.TEST_YANDEX_UID,
                        lite_uid='', fields={'login': settings.TEST_YANDEX_LOGIN,
                                             'login_rule': None},
                        redirect='', emails='', default_email=TEST_YANDEX_EMAIL,
                        status='Some status', error=None, new_session=None, new_sslsession=None,
                        oauth=None, secure=False)

        result = blackbox.odict(defaults, **self.params)

        self.sessionid = Mock(return_value=result)
        self.userinfo = Mock(return_value=result)
        self.user_ticket = Mock(return_value=result)
        self.oauth = Mock(return_value=result)


OAUTH_SETTINGS = dict(
    AUTHENTICATION_BACKENDS=[
        'django_yauth.authentication_mechanisms.oauth.Mechanism',
    ],
    YAUTH_BLACKBOX_INSTANCE=MockBlackbox(oauth=Mock(scope='serve protect')),
    YAUTH_USE_SITES=False,
)


class TestOauth(TestCase):
    factory = RequestFactory(
        SERVER_NAME=settings.TEST_HOST,
        REMOTE_ADDR=settings.TEST_IP,
        SERVER_PORT=443,
        HTTP_AUTHORIZATION='OAuth %s' % settings.TEST_YANDEX_OAUTH_TOKEN,
    )

    @override_settings(**OAUTH_SETTINGS)
    def test_it_authenticates(self):
        user = authenticate(request=self.factory.get(''))
        assert user is not None
        assert user.is_authenticated()
        assert user.authenticated_by.mechanism_name == 'oauth'
        assert user.scopes == {'serve', 'protect'}
        assert user.login == settings.TEST_YANDEX_LOGIN
        assert user.check_scopes('serve') is True
        assert user.check_scopes('get_money') is False

    @override_settings(**OAUTH_SETTINGS)
    def test_it_denies_auth(self):
        user = authenticate(request=self.factory.get('', HTTP_AUTHORIZATION=''))
        assert user is None


COOKIE_SETTINGS = dict(
    AUTHENTICATION_BACKENDS=[
        'django_yauth.authentication_mechanisms.cookie.Mechanism',
    ],
    YAUTH_BLACKBOX_INSTANCE=MockBlackbox(),
    YAUTH_USE_SITES=False,
)

COOKIE_TVM2_SETTINGS = dict(
    AUTHENTICATION_BACKENDS=[
        'django_yauth.authentication_mechanisms.cookie.Mechanism',
    ],
    YAUTH_USE_SITES=False,
    YAUTH_USE_TVM2_FOR_BLACKBOX=True,
    YAUTH_TVM2_CLIENT_ID='28',
    YAUTH_TVM2_SECRET='GRMJrKnj4fOVnvOqe-WyD1',
)

COOKIE_TVM2_SETTINGS_WITH_GET_USER_TICKET = dict(
    AUTHENTICATION_BACKENDS=[
        'django_yauth.authentication_mechanisms.cookie.Mechanism',
    ],
    YAUTH_USE_SITES=False,
    YAUTH_USE_TVM2_FOR_BLACKBOX=True,
    YAUTH_TVM2_CLIENT_ID='28',
    YAUTH_TVM2_SECRET='GRMJrKnj4fOVnvOqe-WyD1',
    YAUTH_TVM2_GET_USER_TICKET=True,
    YAUTH_TVM2_BLACKBOX_CLIENT=BlackboxClientId.Prod,
    YAUTH_BLACKBOX_INSTANCE=MockBlackbox(user_ticket='some_user_ticket'),
)


class TestCookie(TestCase):
    factory = RequestFactory(
        SERVER_NAME=settings.TEST_HOST,
        REMOTE_ADDR=settings.TEST_IP,
        SERVER_PORT=443,
    )

    @property
    def auth_request(self):
        request = self.factory.get('')
        request.COOKIES = {'Session_id': 'some cookie', 'sessionid2': 'secret cookie'}
        return request

    @override_settings(**COOKIE_SETTINGS)
    def test_it_authenticates(self):
        user = authenticate(request=self.auth_request)
        assert user is not None
        assert user.is_authenticated()
        assert user.authenticated_by.mechanism_name == 'cookie'
        assert user.login == settings.TEST_YANDEX_LOGIN

    @override_settings(**COOKIE_TVM2_SETTINGS)
    def test_it_pass_correct_params_to_blackbox(self):
        with patch('blackbox.XmlBlackbox') as patched_blackbox:
            authenticate(request=self.auth_request)
            patched_blackbox.assert_called_once_with(
                blackbox_client=BlackboxClientId.Mimino,
                tvm2_client_id=COOKIE_TVM2_SETTINGS['YAUTH_TVM2_CLIENT_ID'],
                tvm2_secret=COOKIE_TVM2_SETTINGS['YAUTH_TVM2_SECRET'],
                url='http://blackbox-mimino.yandex.net/blackbox',
            )

    @override_settings(**COOKIE_TVM2_SETTINGS_WITH_GET_USER_TICKET)
    def test_authenticate_and_get_user_ticket(self):
        user = authenticate(request=self.auth_request)
        assert user is not None
        assert user.is_authenticated()
        assert user.authenticated_by.mechanism_name == 'cookie'
        assert user.login == settings.TEST_YANDEX_LOGIN
        assert user.raw_user_ticket == 'some_user_ticket'

    @override_settings(**COOKIE_SETTINGS)
    def test_it_denies_auth(self):
        request = self.factory.get('')
        user = authenticate(request=request)
        assert user is None


TVM_SETTINGS = dict(
    AUTHENTICATION_BACKENDS=[
        'django_yauth.authentication_mechanisms.tvm.Mechanism',
    ],
    YAUTH_USE_SITES=False,
    YAUTH_TVM2_CLIENT_ID='28',
    YAUTH_TVM2_SECRET='GRMJrKnj4fOVnvOqe-WyD1',
    YAUTH_MECHANISMS=[
        'django_yauth.authentication_mechanisms.tvm',
    ],
    YAUTH_TVM2_BLACKBOX_CLIENT=BlackboxClientId.Test,
)


VALID_SERVICE_TICKET = (
    '3:serv:CBAQ__________9_IhkI5QEQHBoIYmI6c2VzczEaCGJiOnNlc3My:WUPx1cTf05fjD1exB35T5j2DCHWH1YaLJon_a'
    '4rN-D7JfXHK1Ai4wM4uSfboHD9xmGQH7extqtlEk1tCTCGm5qbRVloJwWzCZBXo3zKX6i1oBYP_89WcjCNPVe1e8jwGdLsnu6'
    'PpxL5cn0xCksiStILH5UmDR6xfkJdnmMG94o8'
)

INVALID_SERVICE_TICKET = (
    '3:serv:CBAQ__________9_czEaCGJiOnNlc3My:WUPx1cTf05fjD1exB35T5j2DCHWH1YaLJon_a4rN-D7JfXHK1Ai4wM4uS'
    'fboHD9xmGQH7extqtlEk1tCTCGm5qbRVloJwWzCZBXo3zKX6i1oBYP_89WcjCNPVe1e8jwGdLsnu6PpxL5cn0xCksiStILH5U'
    'mDR6xfkJdnmMG94o8'
)

VALID_USER_TICKET = (
    '3:user:CA0Q__________9_GiQKAwjIAwoCCHsQyAMaCGJiOnNlc3MxGghiYjpzZXNzMiASKAE:KJFv5EcXn9krYk19LCvlFrhMW'
    '-R4q8mKfXJXCd-RBVBgUQzCOR1Dx2FiOyU-BxUoIsaU0PiwTjbVY5I2onJDilge70Cl5zEPI9pfab2qwklACq_ZBUvD1tzrfNUr88'
    'otBGAziHASJWgyVDkhyQ3p7YbN38qpb0vGQrYNxlk4e2I'
)

INVALID_USER_TICKET = (
    '3:user:CA0Q__________9_GiQKAwjIAwoCCHsQyAMaCGJiOnNlc3MxcXn9krYk19LCvlFrhMW-R4q8mKfXJXCd-RBVBgUQzC'
    'OR1Dx2FiOyU-BxUoIsaU0PiwTjbVY5I2onJDilge70Cl5zEPI9pfab2qwklACq_ZBUvD1tzrfNUr88otBGAziHASJWgyVDkhy'
    'Q3p7YbN38qpb0vGQrYNxlk4e2I'
)


class TestTVM(TestCase):
    factory = RequestFactory(
        SERVER_NAME=settings.TEST_HOST,
        REMOTE_ADDR=settings.TEST_IP,
        SERVER_PORT=443,
    )

    def authenticate(self, request, **credentials):
        with TvmClientPatcher(MockedTvmClient(self_tvm_id=int(TVM_SETTINGS['YAUTH_TVM2_CLIENT_ID']))):
            return authenticate(request, **credentials)

    def auth_request(self, headers):
        request = self.factory.get('', **headers)
        return request

    @override_settings(**TVM_SETTINGS)
    def test_user_token_authenticates_success(self):
        tvm = get_tvm2_client()
        tvm.allowed_clients = (229,)
        headers = {settings.YAUTH_TVM2_SERVICE_HEADER: VALID_SERVICE_TICKET,
                   settings.YAUTH_TVM2_USER_HEADER: VALID_USER_TICKET
                   }
        user = self.authenticate(request=self.auth_request(headers))

        assert user is not None
        assert user.is_authenticated()
        assert user.authenticated_by.mechanism_name == 'tvm'
        assert user.is_impersonated is True
        assert user.uids == [456, 123]
        assert user.uid == 456
        assert user.scopes == ['bb:sess1', 'bb:sess2']
        assert user.check_scopes('bb:sess2') is True
        assert user.check_scopes('bb:fire_imperator') is False
        assert user.raw_service_ticket == VALID_SERVICE_TICKET
        assert user.raw_user_ticket == VALID_USER_TICKET

        assert user.authenticated_by.client_state == TvmClientStatus.Ok

        with pytest.raises(AttributeError):
            # попробуем получить логин без YAUTH_TVM2_GET_USER_INFO
            user.login

    @override_settings(**TVM_SETTINGS)
    def test_user_token_authenticates_success_with_data_from_blackbox(self):
        tvm = get_tvm2_client()
        tvm.allowed_clients = (229,)
        headers = {settings.YAUTH_TVM2_SERVICE_HEADER: VALID_SERVICE_TICKET,
                   settings.YAUTH_TVM2_USER_HEADER: VALID_USER_TICKET
                   }
        mock_blackbox = MockBlackbox()
        with override_settings(
                YAUTH_TVM2_GET_USER_INFO=True,
                YAUTH_BLACKBOX_INSTANCE=mock_blackbox,
        ):
            user = self.authenticate(request=self.auth_request(headers))

        assert user is not None
        assert user.is_authenticated()
        assert user.authenticated_by.mechanism_name == 'tvm'
        assert user.is_impersonated is True
        assert user.uids == [456, 123]
        assert user.uid == 456
        assert user.scopes == ['bb:sess1', 'bb:sess2']
        assert user.check_scopes('bb:sess2') is True
        assert user.check_scopes('bb:fire_imperator') is False
        assert user.raw_service_ticket == VALID_SERVICE_TICKET
        assert user.raw_user_ticket == VALID_USER_TICKET
        assert user.login == 'whitebox-tester'
        mock_blackbox.user_ticket.assert_not_called()
        mock_blackbox.userinfo.assert_called()

    @override_settings(**TVM_SETTINGS)
    def test_user_token_authenticates_success_with_data_from_blackbox_by_user_token(self):
        tvm = get_tvm2_client()
        tvm.allowed_clients = (229,)
        headers = {
            settings.YAUTH_TVM2_SERVICE_HEADER: VALID_SERVICE_TICKET,
            settings.YAUTH_TVM2_USER_HEADER: VALID_USER_TICKET,
        }

        mock_blackbox = MockBlackbox()
        with override_settings(
            YAUTH_TVM2_GET_USER_INFO=True,
            YAUTH_TVM2_USE_USER_TICKET_FOR_USER_INFO=True,
            YAUTH_BLACKBOX_INSTANCE=mock_blackbox,
        ):
            user = self.authenticate(request=self.auth_request(headers))

        assert user is not None
        assert user.is_authenticated()
        assert user.authenticated_by.mechanism_name == 'tvm'
        assert user.is_impersonated is True
        assert user.uids == [456, 123]
        assert user.uid == 456
        assert user.scopes == ['bb:sess1', 'bb:sess2']
        assert user.check_scopes('bb:sess2') is True
        assert user.check_scopes('bb:fire_imperator') is False
        assert user.raw_service_ticket == VALID_SERVICE_TICKET
        assert user.raw_user_ticket == VALID_USER_TICKET
        assert user.login == 'whitebox-tester'
        mock_blackbox.user_ticket.assert_called()
        mock_blackbox.userinfo.assert_not_called()

    @override_settings(**TVM_SETTINGS)
    def test_user_token_authenticates_wrong_service_fail(self):
        tvm = get_tvm2_client()
        tvm.allowed_clients = tuple()
        headers = {settings.YAUTH_TVM2_USER_HEADER: VALID_USER_TICKET,
                   settings.YAUTH_TVM2_SERVICE_HEADER: VALID_SERVICE_TICKET
                   }
        user = self.authenticate(request=self.auth_request(headers))
        assert user is None

    @override_settings(**TVM_SETTINGS)
    def test_user_token_authenticates_no_service_ticket_fail(self):
        headers = {settings.YAUTH_TVM2_USER_HEADER: VALID_USER_TICKET}
        user = self.authenticate(request=self.auth_request(headers))
        assert user is None

    @override_settings(**TVM_SETTINGS)
    def test_user_token_authenticates_fail(self):
        headers = {settings.YAUTH_TVM2_USER_HEADER: INVALID_USER_TICKET}
        user = self.authenticate(request=self.auth_request(headers))
        assert user is None

    @override_settings(**TVM_SETTINGS)
    def test_user_token_authenticates_invalid_user_ticket_fail(self):
        tvm = get_tvm2_client()
        tvm.allowed_clients = (229,)
        headers = {settings.YAUTH_TVM2_SERVICE_HEADER: VALID_SERVICE_TICKET,
                   settings.YAUTH_TVM2_USER_HEADER: INVALID_USER_TICKET
                   }
        user = self.authenticate(request=self.auth_request(headers))
        assert user is None

    @override_settings(**TVM_SETTINGS)
    def test_service_token_authenticates_success(self):
        tvm = get_tvm2_client()
        tvm.allowed_clients = (229, )
        headers = {settings.YAUTH_TVM2_SERVICE_HEADER: VALID_SERVICE_TICKET}
        user = self.authenticate(request=self.auth_request(headers))
        assert user is not None
        assert user.is_authenticated()
        assert user.authenticated_by.mechanism_name == 'tvm'
        assert user.is_impersonated is False
        assert user.service_ticket.src == 229
        assert user.scopes == []
        assert user.raw_service_ticket == VALID_SERVICE_TICKET

    @override_settings(**TVM_SETTINGS)
    def test_service_token_authenticates_no_allowed_service_id_fail(self):
        headers = {settings.YAUTH_TVM2_SERVICE_HEADER: VALID_SERVICE_TICKET}
        user = self.authenticate(request=self.auth_request(headers))
        assert user is None

    @override_settings(**TVM_SETTINGS)
    def test_service_token_authenticates_fail(self):
        headers = {settings.YAUTH_TVM2_SERVICE_HEADER: INVALID_SERVICE_TICKET}
        user = self.authenticate(request=self.auth_request(headers))
        assert user is None

    @override_settings(**TVM_SETTINGS)
    def test_it_denies_auth(self):
        request = self.factory.get('')
        user = self.authenticate(request=request)
        assert user is None
