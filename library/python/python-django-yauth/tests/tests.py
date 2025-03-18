# -*- coding: utf-8 -*-
from __future__ import with_statement

import six
import warnings

if six.PY2:
    from urllib import unquote
elif six.PY3:
    from urllib.parse import unquote

import pytest
from mock import Mock, patch

from django.conf import settings
from django.db import models
from django.db.utils import IntegrityError
from django.template.context import RequestContext
from django.template import Template
from django.contrib.sites.models import Site
from django.test import RequestFactory, TestCase
from django.test.utils import override_settings
from django.http import HttpRequest, HttpResponse, HttpResponseForbidden, HttpResponseRedirect
from django.contrib.auth.models import User
from django.utils.http import urlencode
from django.core.exceptions import ImproperlyConfigured
from django_yauth.context import yauth as yauth_context_processor

try:
    # Django < 1.6
    from django.conf.urls.defaults import url
except ImportError:
    # Django >= 1.6
    from django.conf.urls import url

try:
    # Django >= 1.10
    from django.urls import reverse
except ImportError:
    # Django < 1.10
    from django.core.urlresolvers import reverse

import blackbox

from django_yauth.middleware import YandexAuthMiddleware, YandexAuthTestMiddleware
from django_yauth.context import yauth
from django_yauth import util, urls, views
from django_yauth.decorators import yalogin_required


TEST_YANDEX_EMAIL = '%s@yandex.ru' % settings.TEST_YANDEX_LOGIN


@pytest.fixture(autouse=True)
def create_site():
    Site.objects.filter(id=settings.SITE_ID).update(name='localhost.yandex.ru', domain='localhost.yandex.ru')


class TestProfile(models.Model):
    yandex_uid = models.PositiveIntegerField(primary_key=True)

    class Meta:
        app_label = 'django_yauth'


def _test_view(request):
    return HttpResponse(Template('').render(RequestContext(request)))


urlpatterns = (
    url(r'^$', _test_view),
    url(r'^create-profile/$', views.create_profile, name=settings.YAUTH_CREATE_PROFILE_VIEW),
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
        self.oauth = Mock(return_value=result)

    def _check_login_rule(self, *args, **kwargs):
        return False


def disable_site_app(func):
    return override_settings(YAUTH_USE_SITES=False)(func)


class UtilsTestCase(TestCase):
    def setUp(self):
        """
        Настройка тестовой среды
        """
        self.factory = RequestFactory(HTTP_HOST=settings.TEST_HOST,
                                      REMOTE_ADDR=settings.TEST_IP,
                                      SERVER_PORT=80)

    def test_current_url(self):
        site = Site.objects.all()[0]
        site.name = 'foo.bar:9000'
        site.domain = 'foo.bar:9000'
        site.save()

        p = '/path?key=value'
        self.assertEqual(
            util.get_current_url(self.factory.get(p)),
            b'http://foo.bar:9000/path?key=value'
        )

        with self.settings(YAUTH_USE_SITES=False):
            self.assertEqual(
                util.get_current_url(
                    self.factory.get(p, **{
                        'wsgi.url_scheme': 'https',
                        'SERVER_PORT': 443,
                    })),
                b'https://localhost.yandex.ru/path?key=value'
            )

        self.assertEqual(
            util.get_current_url(
                self.factory.get(p),
                add_params={'a': 'b'},
                del_params=['key']
            ),
            b'http://foo.bar:9000/path?a=b'
        )

        self.assertEqual(
            util.get_current_url(
                self.factory.get('/тыц-тыц?a=b'),
            ),
            u'http://foo.bar:9000/тыц-тыц?a=b'.encode('UTF-8')
        )

        self.assertEqual(
            util.get_current_url(
                self.factory.get('/тыц-тыц'),
                add_params={'a': 'b'},
            ),
            u'http://foo.bar:9000/тыц-тыц?a=b'.encode('UTF-8')
        )

    def test_current_host_site_app(self):
        site = Site.objects.all()[0]
        site.name = 'foo.bar:9000'
        site.domain = 'foo.bar:9000'
        site.save()
        request = self.factory.get('', SERVER_NAME='foo2.bar2')

        self.assertEqual(util.get_current_host(request), 'foo.bar')

    @disable_site_app
    def test_current_host(self):
        request = self.factory.get('', HTTP_HOST='foo2.bar2')

        self.assertEqual(util.get_current_host(request), 'foo2.bar2')


class BaseAuthTestCase(TestCase):
    def setUp(self):
        """
        Настройка тестовой среды
        """
        self.factory = RequestFactory(SERVER_NAME=settings.TEST_HOST,
                                      REMOTE_ADDR=settings.TEST_IP,
                                      SERVER_PORT=80)
        self.com_factory = RequestFactory(SERVER_NAME=settings.TEST_HOST.replace('.ru', '.com', 1),
                                          REMOTE_ADDR=settings.TEST_IP,
                                          SERVER_PORT=80)

    def yandex_login(self, request):
        """
        Получить свежую Session_id для заданного пользователя
        """
        request.COOKIES['Session_id'] = settings.TEST_YANDEX_SESSION_ID

    def yandex_login_by_two_cookies(self, request):
        """
        Получить куки Session_id и sessionid2 для заданного пользователя
        """
        request.COOKIES['Session_id'] = settings.TEST_YANDEX_SESSION_ID
        request.COOKIES['sessionid2'] = settings.TEST_YANDEX_SESSION_ID2

    def yandex_login_by_sessionid_and_sessguard(self, request):
        """
        Получить куки Session_id и sessguard для заданного пользователя
        """
        request.COOKIES['Session_id'] = settings.TEST_YANDEX_SESSION_ID
        request.COOKIES['sessguard'] = settings.TEST_YANDEX_SESSGUARD


class MiddlewareTest(BaseAuthTestCase):
    """
    Тест middleware. Тестируется добавление в запрос объекта
    с данными о пользователе
    """

    @override_settings(YAUTH_BLACKBOX_INSTANCE=MockBlackbox())
    def test_guest(self):
        """
        Тест анонимного пользователя
        """
        request = self.factory.get('')
        YandexAuthMiddleware().process_request(request)
        self.assertEqual(request.yauser.is_authenticated(), False)
        self.assertEqual(request.client_application.id, None)

        self.assertFalse(settings.YAUTH_BLACKBOX_INSTANCE.sessionid.called)
        self.assertFalse(settings.YAUTH_BLACKBOX_INSTANCE.oauth.called)

    @override_settings(YAUTH_BLACKBOX_INSTANCE=MockBlackbox())
    def test_authenticated(self):
        """
        Тест залогиненного пользователя.
        """
        request = self.factory.get('')
        self.yandex_login(request)
        YandexAuthMiddleware().process_request(request)

        self.assertEqual(request.yauser.is_authenticated(), True)
        self.assertFalse(request.yauser.is_lite())
        self.assertEqual(request.yauser.login, settings.TEST_YANDEX_LOGIN)
        self.assertEqual(request.yauser.uid, settings.TEST_YANDEX_UID)

        settings.YAUTH_BLACKBOX_INSTANCE.sessionid.assert_called_once_with(
            settings.TEST_YANDEX_SESSION_ID, settings.TEST_IP, 'localhost.yandex.ru', sslsessionid=None,
            emails='getdefault', dbfields=settings.YAUTH_PASSPORT_FIELDS)

    def _process_oauth_request(self, bearer):
        request = self.factory.get('', HTTP_AUTHORIZATION='%s %s' % (
            bearer, settings.TEST_YANDEX_OAUTH_TOKEN))
        YandexAuthMiddleware().process_request(request)
        return request

    def _authenticated_oauth(self, bearer):
        request = self._process_oauth_request(bearer)

        self.assertEqual(request.yauser.is_authenticated(), True)
        self.assertEqual(request.yauser.is_lite(), False)
        self.assertEqual(request.yauser.login, settings.TEST_YANDEX_LOGIN)
        self.assertEqual(request.yauser.uid, settings.TEST_YANDEX_UID)
        self.assertEqual(request.yauser.oauth.client_name, settings.TEST_YANDEX_APPLICATION_NAME)
        self.assertEqual(request.client_application.name, settings.TEST_YANDEX_APPLICATION_NAME)
        self.assertEqual(request.client_application.expire_time, None)
        self.assertEqual(request.client_application.scope, ['foo:bar', 'bar:foo'])

        settings.YAUTH_BLACKBOX_INSTANCE.oauth.assert_called_with(
            settings.TEST_YANDEX_OAUTH_TOKEN, settings.TEST_IP,
            by_token=True, emails='getdefault', dbfields=settings.YAUTH_PASSPORT_FIELDS)

    @override_settings(
        YAUTH_BLACKBOX_INSTANCE=MockBlackbox(
            oauth=blackbox.odict(
                client_id=1,
                client_name=settings.TEST_YANDEX_APPLICATION_NAME,
                client_homepage='',
                client_icon='',
                uid=settings.TEST_YANDEX_UID,
                ctime='2012-11-30 15:55:55',
                issue_time='2012-11-30 15:55:55',
                expire_time='None',
                scope='foo:bar bar:foo',
            ),
        ),
        YAUTH_MECHANISMS=[
            'django_yauth.authentication_mechanisms.oauth',
        ],
    )
    def test_oauth_bearer(self):
        for bearer in ('OAuth', 'Bearer'):
            self._authenticated_oauth(bearer)

        for bearer in ('token', 'ololo'):
            with override_settings(YAUTH_OAUTH_HEADER_PATTERN_PREFIX=r'token|ololo'):
                self._authenticated_oauth(bearer)

        with override_settings(YAUTH_OAUTH_AUTHORIZATION_SCOPES=['foo:bar', 'qwe:qwe']):
            self._authenticated_oauth('OAuth')

        with override_settings(YAUTH_OAUTH_AUTHORIZATION_SCOPES=['scope:fail']):
            request = self._process_oauth_request('oauth')
            self.assertEqual(request.yauser.is_authenticated(), False)

    @override_settings(YAUTH_BLACKBOX_INSTANCE=MockBlackbox(emails=[blackbox.odict(address=TEST_YANDEX_EMAIL,
                                                                                   default=True)]),
                       YAUTH_BLACKBOX_PARAMS={'emails': 'getdefault'})
    def test_emails(self):
        request = self.factory.get('')
        self.yandex_login(request)
        YandexAuthMiddleware().process_request(request)

        self.assertEqual(request.yauser.emails[0].address, TEST_YANDEX_EMAIL)
        self.assertEqual(request.yauser.default_email, TEST_YANDEX_EMAIL)

    @patch('django_yauth.authentication_mechanisms.cookie.generate_tvm_ts_sign', lambda x: ('ts-test', 'ts_sign-test'))
    @override_settings(YAUTH_BLACKBOX_INSTANCE=MockBlackbox(),
                       YAUTH_TVM_CLIENT_ID=555,
                       YAUTH_TVM_CLIENT_SECRET='fake',
                       YAUTH_BLACKBOX_CONSUMER='tests')
    def test_tvm(self):
        request = self.factory.get('')
        self.yandex_login(request)
        YandexAuthMiddleware().process_request(request)

        settings.YAUTH_BLACKBOX_INSTANCE.sessionid.assert_called_once_with(
            settings.TEST_YANDEX_SESSION_ID, settings.TEST_IP, 'localhost.yandex.ru',
            sslsessionid=None, emails='getdefault', dbfields=settings.YAUTH_PASSPORT_FIELDS,
            getticket='yes', client_id=555, consumer='tests', ts='ts-test', ts_sign='ts_sign-test')

    @override_settings(YAUTH_BLACKBOX_INSTANCE=MockBlackbox(secure=True), YAUTH_SESSIONID2_REQUIRED=True)
    def test_authenticated_with_check_of_two_cookies(self):
        """
        Тест авторизации пользователя по двум кукам: Session_id и sessionid2.
        """
        request = self.factory.get('', **{'wsgi.url_scheme': 'https'})
        self.yandex_login_by_two_cookies(request)
        resp = YandexAuthMiddleware().process_request(request)

        self.assertEqual(resp, None)
        self.assertEqual(request.yauser.is_authenticated(), True)
        self.assertFalse(request.yauser.is_lite())
        self.assertEqual(request.yauser.login, settings.TEST_YANDEX_LOGIN)
        self.assertEqual(request.yauser.uid, settings.TEST_YANDEX_UID)

        settings.YAUTH_BLACKBOX_INSTANCE.sessionid.assert_called_once_with(
            settings.TEST_YANDEX_SESSION_ID, settings.TEST_IP, 'localhost.yandex.ru',
            sslsessionid=settings.TEST_YANDEX_SESSION_ID2,
            emails='getdefault', dbfields=settings.YAUTH_PASSPORT_FIELDS)

    @override_settings(YAUTH_BLACKBOX_INSTANCE=MockBlackbox(), YAUTH_SESSIONID2_REQUIRED=True)
    def test_authenticated_by_two_cookies_with_invalid_second(self):
        """
        Тест авторизации пользователя по двум кукам: Session_id и sessionid2.
        Вторая кука невалидна, при этом ожидается редирект на паспорт.
        """
        request = self.factory.get('', **{'wsgi.url_scheme': 'https'})
        self.yandex_login_by_two_cookies(request)
        resp = YandexAuthMiddleware().process_request(request)

        assert resp
        assert resp.status_code == 302
        self.assertTrue(resp.get('Location').startswith(util.get_passport_url('create', settings.YAUTH_TYPE, request)))

        settings.YAUTH_BLACKBOX_INSTANCE.sessionid.assert_called_once_with(
            settings.TEST_YANDEX_SESSION_ID, settings.TEST_IP, 'localhost.yandex.ru',
            sslsessionid=settings.TEST_YANDEX_SESSION_ID2,
            emails='getdefault', dbfields=settings.YAUTH_PASSPORT_FIELDS)

    @override_settings(YAUTH_BLACKBOX_INSTANCE=MockBlackbox(), YAUTH_SESSIONID2_REQUIRED=True)
    def test_authenticated_by_two_cookies_with_invalid_protocol(self):
        """
        Тест авторизации пользователя по двум кукам с невалидным для этого случая протоколом HTTP.
        """
        request = self.factory.get('')
        self.yandex_login_by_two_cookies(request)
        resp = YandexAuthMiddleware().process_request(request)

        self.assertTrue(resp)
        self.assertEqual(resp.status_code, 400)
        self.assertTrue(resp.content, 'Authorization error: HTTPS connection is required')

    @override_settings(YAUTH_BLACKBOX_INSTANCE=MockBlackbox(secure=True))
    def test_authenticated_with_check_of_sessionid_and_sessguard(self):
        """
        Тест авторизации пользователя по двум кукам: Session_id и sessguard.
        """
        request = self.factory.get('', **{'wsgi.url_scheme': 'https'})
        self.yandex_login_by_sessionid_and_sessguard(request)
        resp = YandexAuthMiddleware().process_request(request)

        self.assertEqual(resp, None)
        self.assertEqual(request.yauser.is_authenticated(), True)
        self.assertFalse(request.yauser.is_lite())
        self.assertEqual(request.yauser.login, settings.TEST_YANDEX_LOGIN)
        self.assertEqual(request.yauser.uid, settings.TEST_YANDEX_UID)

        settings.YAUTH_BLACKBOX_INSTANCE.sessionid.assert_called_once_with(
            settings.TEST_YANDEX_SESSION_ID, settings.TEST_IP, 'localhost.yandex.ru',
            sslsessionid=None, sessguard=settings.TEST_YANDEX_SESSGUARD,
            emails='getdefault', dbfields=settings.YAUTH_PASSPORT_FIELDS)


class YauserTagTest(BaseAuthTestCase):
    """
    Тест тега yauth

    В тестах рендерится небольшой шаблон и проверяется наличие
    CSS-классов.
    """
    @pytest.mark.skip('Need to fix this in order to work with arcadia tests')
    def test_guest(self):
        """
        Тест анонимного пользователя
        """
        request = self.factory.get('')
        YandexAuthMiddleware().process_request(request)

        template = Template('{% load yauth %}{% yauth %}')
        text = template.render(RequestContext(request))
        self.assertTrue('a class="enter"' in text)

    @pytest.mark.skip('Need to fix this in order to work with arcadia tests')
    @override_settings(YAUTH_BLACKBOX_INSTANCE=MockBlackbox())
    def test_authenticated(self):
        """
        Тест залогиненного пользователя.

        В тесте специально получается свежий Session_id для тестового
        пользователя
        """
        request = self.factory.get('')
        self.yandex_login(request)
        YandexAuthMiddleware().process_request(request)

        template = Template('{% load yauth %}{% yauth %}')

        try:
            from django.template.context_processors import request as req_proccessor
        except ImportError:  # django 1.6
            from django.core.context_processors import request as req_proccessor

        tmp_ctx = RequestContext(request, processors=[req_proccessor, yauth_context_processor])

        text = template.render(tmp_ctx)

        assert 'a class="user"' in text
        assert '<b>w</b>hitebox-tester' in text


class YauserContextTest(BaseAuthTestCase):
    """
    Тест context-процессора
    """

    @disable_site_app
    def test_passport_urls(self):
        """
        Тест для проверки ссылок в зависимости от YAUTH_TYPE
        """
        with self.settings(YAUTH_TYPE='mobile'):
            request = self.factory.get('')
            context = yauth(request)
            self.assertTrue(any(['://pda-passport.yandex.ru' in context[x]
                                     for x in ['login_url', 'logout_url', 'register_url', 'passport_account_url', 'passport_host']]))
        with self.settings(YAUTH_TYPE='desktop'):
            request = self.factory.get('')
            context = yauth(request)
            self.assertTrue(any(['://passport.yandex.ru' in context[x]
                                 for x in ['login_url', 'logout_url', 'register_url', 'passport_account_url', 'passport_host']]))
        with self.settings(YAUTH_TYPE='intranet'):
            request = self.factory.get('')
            context = yauth(request)
            self.assertTrue(any(['://passport.yandex-team.ru' in context[x]
                                     for x in ['login_url', 'logout_url', 'passport_account_url', 'passport_host']]))
            self.assertFalse('register_url' in context)
        with self.settings(YAUTH_TYPE='intranet-testing'):
            request = self.factory.get('')
            context = yauth(request)
            self.assertTrue(any(['://passport-test.yandex-team.ru' in context[x]
                                     for x in ['login_url', 'logout_url', 'passport_account_url', 'passport_host']]))
            self.assertFalse('register_url' in context)

    @disable_site_app
    def test_passport_urls_com(self):
        """
        Тест для проверки ссылок в зависимости от YAUTH_TYPE
        """
        with self.settings(YAUTH_TYPE='mobile'):
            request = self.com_factory.get('')
            context = yauth(request)
            self.assertTrue(any(['://pda-passport.yandex.com' in context[x]
                                     for x in ['login_url', 'logout_url', 'register_url', 'passport_account_url', 'passport_host']]))
        with self.settings(YAUTH_TYPE='desktop'):
            request = self.com_factory.get('')
            context = yauth(request)
            self.assertTrue(any(['://passport.yandex.com' in context[x]
                                 for x in ['login_url', 'logout_url', 'register_url', 'passport_account_url', 'passport_host']]))
        with self.settings(YAUTH_TYPE='intranet'):
            request = self.com_factory.get('')
            context = yauth(request)
            # Ожидаем .ru, т.к. нет интранетного com-паспорта
            self.assertTrue(any(['://passport.yandex-team.ru' in context[x]
                                 for x in ['login_url', 'logout_url', 'passport_account_url', 'passport_host']]))
            self.assertFalse('register_url' in context)
        with self.settings(YAUTH_TYPE='intranet-testing'):
            request = self.com_factory.get('')
            context = yauth(request)
            # Ожидаем .ru, т.к. нет интранетного com-паспорта
            self.assertTrue(any(['://passport-test.yandex-team.ru' in context[x]
                                 for x in ['login_url', 'logout_url', 'passport_account_url', 'passport_host']]))
            self.assertFalse('register_url' in context)


class ProfileTest(BaseAuthTestCase):
    def setUp(self):
        self.request = HttpRequest()
        self.request.META.update({
            'SERVER_NAME': settings.TEST_HOST,
            'REMOTE_ADDR': settings.TEST_IP,
            'SERVER_PORT': 80,
        })

        self.old_setting = settings.YAUTH_YAUSER_PROFILES
        settings.YAUTH_YAUSER_PROFILES = [
            'django_yauth.TestProfile',
        ]

        from django_yauth import models as yauth_models
        yauth_models.add_reference(TestProfile)

    def tearDown(self):
        settings.YAUTH_YAUSER_PROFILES = self.old_setting

    @override_settings(YAUTH_BLACKBOX_INSTANCE=MockBlackbox())
    def test_profile(self):
        self.yandex_login(self.request)
        YandexAuthMiddleware().process_request(self.request)

        # достаем атрибут чтобы профиль создался
        self.request.yauser.django_yauth_testprofile

        # проверяем создание
        self.assertEqual(TestProfile.objects.count(), 1)

        profile = TestProfile.objects.get()

        # проверяем дескриптер юзера у профиля
        self.assertEqual(profile.yauser.uid, self.request.yauser.uid)

        # проверяем флаг лайт-пользователя
        self.assertFalse(profile.yauser.is_lite())

    @patch.object(TestProfile, 'profile_created', Mock(), create=True)
    @override_settings(YAUTH_BLACKBOX_INSTANCE=MockBlackbox())
    def test_should_call_profile_created_if_profile_created(self):
        self.yandex_login(self.request)
        YandexAuthMiddleware().process_request(self.request)

        with patch.object(TestProfile.objects, 'get', Mock(side_effect=TestProfile.DoesNotExist)):
            # достаем атрибут чтобы профиль создался
            profile = self.request.yauser.django_yauth_testprofile

            # проверяем, что метод profile_created вызвался
            self.assertTrue(profile.profile_created.called)

    @patch.object(TestProfile, 'profile_created', Mock(), create=True)
    @override_settings(YAUTH_BLACKBOX_INSTANCE=MockBlackbox())
    def test_should_not_call_profile_created_if_profile_not_created(self):
        self.yandex_login(self.request)
        YandexAuthMiddleware().process_request(self.request)

        TestProfile.objects.create(yandex_uid=self.request.yauser.uid)

        with patch.object(TestProfile.objects, 'get', Mock(side_effect=TestProfile.DoesNotExist)):
            # пытаемся достать атрибут профиля, при этом он не должен создаться
            profile = self.request.yauser.django_yauth_testprofile

            # проверяем, что метод profile_created не вызвался
            self.assertFalse(profile.profile_created.called)

    @override_settings(YAUTH_BLACKBOX_INSTANCE=MockBlackbox())
    def test_should_use_get_or_create_when_create_profile(self):
        self.yandex_login(self.request)
        YandexAuthMiddleware().process_request(self.request)

        TestProfile.objects.create(yandex_uid=self.request.yauser.uid)

        with patch.object(TestProfile.objects, 'get', Mock(side_effect=TestProfile.DoesNotExist)):
            try:
                # достаем атрибут, чтобы профиль попытался создаться
                self.request.yauser.django_yauth_testprofile
                is_raised_not_unique_exception = False
            except IntegrityError:
                is_raised_not_unique_exception = True

            msg = ('при создании профиля должен использоваться метод get_or_create, т.к. отдельная проверка '
                   'на существование профиля может возвращать неконсистентные данные из слэйвов. При этом get_or_create '
                   'всегда производит чтение из мастера.')
            self.assertFalse(is_raised_not_unique_exception, msg=msg)


class MockBlackboxTest(BaseAuthTestCase):
    """
    Тест возможности подменить blackbox.
    """
    @override_settings(YAUTH_BLACKBOX_INSTANCE=MockBlackbox())
    def test_authenticated(self):
        request = self.factory.get('')
        self.yandex_login(request)
        YandexAuthMiddleware().process_request(request)

        self.assertEqual(request.yauser.is_authenticated(), True)
        self.assertEqual(request.yauser.uid, settings.TEST_YANDEX_UID)


class DecoratorTest(BaseAuthTestCase):
    """
    Тест декоратора yalogin_required.
    """
    def setUp(self):
        super(DecoratorTest, self).setUp()

        @yalogin_required
        def view(request):
            return 'Access granted'
        self.view = view

    def get_response(self, request, cookie, from_passport):
        if cookie:
            self.yandex_login(request)
        if from_passport:
            request.GET = request.GET.copy()
            request.GET['_yauth'] = 1

        middleware = YandexAuthMiddleware()
        try:
            middleware.process_request(request)
            return self.view(request)
        except Exception as ex:
            return middleware.process_exception(request, ex)

    @disable_site_app
    def test_redirect(self):
        """Анонимного пользователя должны редиректить в Паспорт"""
        request = self.factory.get('')
        response = self.get_response(request, cookie=False, from_passport=False)
        self.assertTrue(isinstance(response, HttpResponseRedirect))
        self.assertEqual(unquote(response['Location']),
                         'https://passport.yandex.ru/auth?retpath=http://localhost.yandex.ru/?_yauth=1')

        with self.settings(YAUTH_TYPE='intranet'):
            request = self.factory.get('')
            response = self.get_response(request, cookie=False, from_passport=False)
            self.assertTrue(isinstance(response, HttpResponseRedirect))
            self.assertEqual(unquote(response['Location']),
                             'https://passport.yandex-team.ru/auth?retpath=http://localhost.yandex.ru/?_yauth=1')

        with self.settings(YAUTH_TYPE='intranet-testing'):
            request = self.factory.get('')
            response = self.get_response(request, cookie=False, from_passport=False)
            self.assertTrue(isinstance(response, HttpResponseRedirect))
            self.assertEqual(unquote(response['Location']),
                             'https://passport-test.yandex-team.ru/auth?retpath=http://localhost.yandex.ru/?_yauth=1')

        request = self.com_factory.get('')
        response = self.get_response(request, cookie=False, from_passport=False)
        self.assertTrue(isinstance(response, HttpResponseRedirect))
        self.assertEqual(unquote(response['Location']),
                         'https://passport.yandex.com/auth?retpath=http://localhost.yandex.com/?_yauth=1')

    @override_settings(YAUTH_BLACKBOX_INSTANCE=MockBlackbox())
    def test_authenticated(self):
        """Залогиненного пользователя должны пустить"""
        request = self.factory.get('')
        response = self.get_response(request, cookie=True, from_passport=False)
        self.assertEqual(response, 'Access granted')

    @disable_site_app
    @override_settings(YAUTH_BLACKBOX_INSTANCE=MockBlackbox())
    def test_just_authenticated(self):
        """Только что залогиненного должны средиректить без _yauth"""
        request = self.factory.get('')
        response = self.get_response(request, cookie=True, from_passport=True)
        self.assertTrue(isinstance(response, HttpResponseRedirect))
        self.assertEqual(response['Location'], 'http://localhost.yandex.ru/')

        request = self.com_factory.get('')
        response = self.get_response(request, cookie=True, from_passport=True)
        self.assertTrue(isinstance(response, HttpResponseRedirect))
        self.assertEqual(response['Location'], 'http://localhost.yandex.com/')

    @override_settings(DEBUG=True, YAUTH_BLACKBOX_INSTANCE=MockBlackbox(valid=False, status='WRONG_GUARD'))
    def test_needs_new_sessguard(self):
        """У пользователя невалидная кука-оберег, нужно отправить в Паспорт за новой"""
        request = self.factory.get('')
        response = self.get_response(request, cookie=True, from_passport=False)
        self.assertTrue(isinstance(response, HttpResponseRedirect))
        self.assertEqual(unquote(response['Location']),
                         'https://passport.yandex.ru/auth/guard?retpath=http://localhost.yandex.ru/?_yauth=1')

    @override_settings(DEBUG=True, YAUTH_BLACKBOX_INSTANCE=MockBlackbox(valid=False))
    def test_debug_blackbox(self):
        """Ошибки блэкбокса должны быть видны в браузере"""
        request = self.factory.get('')
        response = self.get_response(request, cookie=True, from_passport=True)
        self.assertTrue(isinstance(response, HttpResponseForbidden))
        assert b'Invalid response from mock-blackbox' in response.content

    @override_settings(DEBUG=True)
    def test_debug_cookie(self):
        """Должны сообщить, если после паспорта не осталось куки"""
        request = self.factory.get('')
        response = self.get_response(request, cookie=False, from_passport=True)
        self.assertTrue(isinstance(response, HttpResponseForbidden))
        self.assertTrue(b'cookie' in response.content)


class UserTest(BaseAuthTestCase):
    """
    Тест автоматического создания Django пользователя.
    """
    def setUp(self):
        super(UserTest, self).setUp()

        self.request = self.factory.get('/')

    @override_settings(YAUTH_BLACKBOX_INSTANCE=MockBlackbox(),
                       YAUTH_CREATE_USER_ON_ACCESS=True)
    def test_create_user_by_login(self):
        self.yandex_login(self.request)
        YandexAuthMiddleware().process_request(self.request)

        views.create_profile(self.request)

        self.assertEqual(User.objects.filter(username=settings.TEST_YANDEX_LOGIN).count(),
                         1)

    @override_settings(YAUTH_BLACKBOX_INSTANCE=MockBlackbox(),
                       YAUTH_CREATE_USER_ON_ACCESS=True,
                       YAUTH_YAUSER_USERNAME_FIELD='uid')
    def test_create_user_by_uid(self):
        self.yandex_login(self.request)
        YandexAuthMiddleware().process_request(self.request)

        # Патчим User.USERNAME_FIELD для сохранения по 'id'
        with patch.object(User, 'USERNAME_FIELD', new='id'):
            views.create_profile(self.request)

        self.assertEqual(User.objects.filter(id=settings.TEST_YANDEX_UID).count(),
                         1)

    @override_settings(YAUTH_BLACKBOX_INSTANCE=MockBlackbox(),
                       YAUTH_CREATE_USER_ON_ACCESS=True,
                       YAUTH_USER_EXTRA_FIELDS=(('login', 'first_name'), ('login', 'last_name')))
    def test_create_user_with_extra_fields(self):
        self.yandex_login(self.request)
        YandexAuthMiddleware().process_request(self.request)

        views.create_profile(self.request)

        self.assertEqual(User.objects
                         .filter(username=settings.TEST_YANDEX_LOGIN,
                                 first_name=settings.TEST_YANDEX_LOGIN,
                                 last_name=settings.TEST_YANDEX_LOGIN)
                         .count(),
                         1)

    @override_settings(YAUTH_BLACKBOX_INSTANCE=MockBlackbox(default_email=None),
                       YAUTH_CREATE_USER_ON_ACCESS=True,)
    def test_create_user_without_email(self):
        self.yandex_login(self.request)
        YandexAuthMiddleware().process_request(self.request)

        views.create_profile(self.request)

        self.assertEqual(User.objects.get(username=settings.TEST_YANDEX_LOGIN).email, '')


class RedirectTest(BaseAuthTestCase):
    urls = urls

    def setUp(self):
        super(RedirectTest, self).setUp()

        self.request = self.factory.get('/', data={'next': '/come-back/'})

    def test_reversion(self):
        url = reverse(settings.YAUTH_CREATE_PROFILE_VIEW, urlconf=urls)

        self.assertTrue(len(url))

    @override_settings(YAUTH_BLACKBOX_INSTANCE=MockBlackbox())
    def test_view_no_create(self):
        self.yandex_login(self.request)
        response = views.create_profile(self.request)

        self.assertEqual(response.status_code, 302)
        self.assertEqual(response['Location'], '/come-back/')

        self.assertRaises(User.DoesNotExist,
                          lambda: User.objects.get(username=settings.TEST_YANDEX_LOGIN))

    @override_settings(YAUTH_BLACKBOX_INSTANCE=MockBlackbox())
    def test_view_no_create_not_authenticated(self):
        response = views.create_profile(self.request)

        self.assertEqual(response.status_code, 403)

    @override_settings(YAUTH_BLACKBOX_INSTANCE=MockBlackbox(),
                       YAUTH_CREATE_USER_ON_ACCESS=True)
    def test_create_user_redirect(self):
        self.yandex_login(self.request)
        YandexAuthMiddleware().process_request(self.request)

        response = views.create_profile(self.request)

        self.assertEqual(response.status_code, 302)
        self.assertEqual(response['Location'], '/come-back/')

    @override_settings(YAUTH_BLACKBOX_INSTANCE=MockBlackbox(),
                       YAUTH_YAUSER_PROFILES=['django_yauth.TestProfile'])
    def test_create_profile(self):
        self.yandex_login(self.request)
        YandexAuthMiddleware().process_request(self.request)

        response = views.create_profile(self.request)

        self.assertEqual(TestProfile.objects.filter(yandex_uid=settings.TEST_YANDEX_UID).count(),
                         1)

        self.assertEqual(response.status_code, 302)
        self.assertEqual(response['Location'], '/come-back/')

    @override_settings(YAUTH_BLACKBOX_INSTANCE=MockBlackbox(),
                       YAUTH_CREATION_REDIRECT=True,
                       YAUTH_CREATE_USER_ON_ACCESS=True)
    def test_redirect(self):
        self.yandex_login(self.request)
        response = YandexAuthMiddleware().process_request(self.request)

        self.assertEqual(response.status_code, 302)
        self.assertEqual(response['Location'],
                         '/create-profile/?' + urlencode({'next': '/?' + urlencode({'next': '/come-back/'})}))


class UtilsTestCase2(BaseAuthTestCase):
    @disable_site_app
    def test_refresh_url(self):
        request = self.factory.get('')

        self.assertEqual(util.get_passport_url('refresh', request=request),
                         'https://passport.yandex.ru/auth/update?retpath=')

        request = self.com_factory.get('')

        self.assertEqual(util.get_passport_url('refresh', request=request),
                         'https://passport.yandex.com/auth/update?retpath=',)

    def test_warnings(self):
        warnings.resetwarnings()

        request = self.factory.get('')

        with warnings.catch_warnings(record=True) as w:
            util.current_url(request)

            self.assertEqual(len(w), 1)

        with warnings.catch_warnings(record=True) as w:
            util.current_host(request)

            self.assertEqual(len(w), 1)


class TestTestMiddleware(BaseAuthTestCase):
    def test_test_yauser_raise(self):
        request = self.factory.get('')

        self.assertRaises(ImproperlyConfigured,
                          YandexAuthTestMiddleware().process_request, request)

    @override_settings(YAUTH_TEST_USER=False)
    def test_test_yauser_anonymous(self):
        request = self.factory.get('')

        YandexAuthTestMiddleware().process_request(request)

        self.assertFalse(request.yauser.is_authenticated())

    @override_settings(YAUTH_TEST_USER=settings.TEST_YANDEX_LOGIN)
    def test_test_yauser(self):
        request = self.factory.get('')

        YandexAuthTestMiddleware().process_request(request)

        self.assertTrue(request.yauser.is_authenticated())
        self.assertEqual(request.yauser.login, settings.TEST_YANDEX_LOGIN)

    @override_settings(YAUTH_TEST_USER={'login': settings.TEST_YANDEX_LOGIN,
                                        'uid': settings.TEST_YANDEX_UID})
    def test_test_yauser_dict(self):
        request = self.factory.get('')

        YandexAuthTestMiddleware().process_request(request)

        self.assertTrue(request.yauser.is_authenticated())
        self.assertEqual(request.yauser.login, settings.TEST_YANDEX_LOGIN)
        self.assertEqual(request.yauser.uid, settings.TEST_YANDEX_UID)

    @override_settings(YAUTH_TEST_USER=settings.TEST_YANDEX_LOGIN,
                       YAUTH_TEST_APPLICATION=settings.TEST_YANDEX_APPLICATION_NAME)
    def test_test_client_application(self):
        request = self.factory.get('')

        YandexAuthTestMiddleware().process_request(request)

        self.assertEqual(request.client_application.id,
                         settings.TEST_YANDEX_APPLICATION_NAME)
