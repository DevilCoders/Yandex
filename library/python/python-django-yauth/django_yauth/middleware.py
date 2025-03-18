# -*- coding:utf-8 -*-

from django.http import HttpResponseRedirect, HttpResponseForbidden, HttpResponseBadRequest, HttpResponse
from django.conf import settings
from django import VERSION
from django.utils.http import urlencode
from django.contrib.auth import authenticate

from django_yauth.user import (YandexUserDescriptor, DjangoUserDescriptor, ApplicationDescriptor,
                               profile_should_be_created, YandexTestUserDescriptor,
                               TestApplicationDescriptor)
from django_yauth.util import (get_current_url, get_current_host, get_passport_url,
                               debug_msg, get_setting, get_yauth_type)

try:
    from django.utils.deprecation import MiddlewareMixin
except ImportError:
    MiddlewareMixin = object

try:
    # Django >= 1.10
    from django.urls import reverse
except ImportError:
    # Django < 1.10
    from django.core.urlresolvers import reverse

from . import exceptions


class YandexAuthBackendMiddleware(MiddlewareMixin):
    def process_request(self, request):
        request.yauser = authenticate(request=request)


class YandexAuthMiddleware(MiddlewareMixin):
    '''
    Создает request.yauser -- ссылку на авторизованного
    пользователя Яндекса.
    '''
    def process_request(self, request):
        yauth_type = get_yauth_type(request)

        self.assign_yauser(request)
        self.assign_client_application(request)
        self.assign_user(request)

        try:
            version = VERSION[0] * 10 + VERSION[1]
            if (get_setting(['REFRESH_SESSION', 'YAUTH_REFRESH_SESSION'])
                and request.method == 'GET'
                and not request.GET.get('nocookiesupport', False)
                and request.yauser.is_authenticated()
                and request.yauser.need_reset
                and (version <= 30 and not request.is_ajax() or version > 30
                     and request.headers.get('x-requested-with') != 'XMLHttpRequest')):

                response = HttpResponseRedirect(get_passport_url('refresh', yauth_type, request=request, retpath=True))

                domain = get_passport_url('passport_domain', yauth_type, request=request)
                if domain == get_current_host(request):
                    domain = None
                else:
                    domain = domain[domain.find('.'):]

                response.set_cookie(get_setting(['COOKIE_CHECK_COOKIE_NAME', 'YAUTH_COOKIE_CHECK_COOKIE_NAME']),
                                    'Cookie_check', None, None, '/', domain)

                return response

            if (settings.YAUTH_CREATION_REDIRECT
                    and request.method in settings.YAUTH_CREATION_REDIRECT_METHODS
                    and request.path != reverse(settings.YAUTH_CREATE_PROFILE_VIEW)
                    and profile_should_be_created(request)):
                return self.creation_redirect(request)
        except exceptions.AuthException as ex:
            return self.process_exception(request, ex)

    def creation_redirect(self, request):
        url = request.get_full_path()

        return HttpResponseRedirect(reverse(settings.YAUTH_CREATE_PROFILE_VIEW)
                                    + '?' + urlencode({'next': url}))

    def process_exception(self, request, exception):
        if isinstance(exception, (exceptions.TwoCookiesRequired, exceptions.AuthRequired)):
            retpath = getattr(exception, 'retpath', True)
            passport_url = get_passport_url('create', get_yauth_type(request), request=request, retpath=retpath)

            return HttpResponseRedirect(passport_url)
        elif isinstance(exception, exceptions.InvalidProtocol):
            # если сервис требует для авторизации HTTPS соединение, но при этом позволяет запросы по HTTP,
            # то это ошибка сервиса.
            return HttpResponseBadRequest(exception)

    def _set_cookie(self, response, new_session, cookie_name, secure=None):
        response.set_cookie(
            cookie_name,           # key
            new_session.id,        # value
            None,                  # max_age
            new_session.expires,   # expires
            '/',                   # path
            new_session.domain,    # domain
            secure,                # secure
            new_session.http_only  # HttpOnly
        )

    def assign_yauser(self, request):
        request.__class__.yauser = YandexUserDescriptor()

    def assign_client_application(self, request):
        request.__class__.client_application = ApplicationDescriptor()

    def assign_user(self, request):
        if get_setting(['YAUSER_ADMIN_LOGIN', 'YAUTH_USE_NATIVE_USER']):
            request.__class__.user = DjangoUserDescriptor()


class YandexAuthRequiredMiddleware(MiddlewareMixin):
    '''
    Требует наличие авторизованного яндексового пользователя.
    '''
    def process_request(self, request):
        if request.yauser.is_authenticated():
            if settings.YAUTH_ANTI_REPEAT_PARAM in request.GET:
                # Убираем YAUTH_ANTI_REPEAT_PARAM, чтобы не ломать ссылку Выйти.
                return HttpResponseRedirect(
                    get_current_url(request, del_params=[settings.YAUTH_ANTI_REPEAT_PARAM]))
        else:
            if request.method == 'GET' and not request.yauser.oauth:
                if settings.YAUTH_ANTI_REPEAT_PARAM in request.GET:
                    # чтобы не редиректить бесконечно на Паспорт
                    return HttpResponseForbidden('Authorization failed' +
                                                 debug_msg(request) if settings.DEBUG else '')

                retpath = get_current_url(request, add_params={settings.YAUTH_ANTI_REPEAT_PARAM: 1})
                url_type = 'create_guard' if request.yauser.needs_new_sessguard() else 'create'
                passport_url = get_passport_url(url_type, get_yauth_type(request), request=request, retpath=retpath)

                return HttpResponseRedirect(passport_url)
            elif request.yauser.blackbox_result is None:
                return HttpResponse('Authentication required', status=401)
            else:
                # Случается в редких случаях, когда юзер до-о-олго сидела на
                # странице с открытой формой, и авторизация протухла.
                # Спасти данные мы уже не можем.

                # TODO: бросать PermissionDenied, когда перейдем на django>=1.4
                return HttpResponseForbidden('Authorization failed')


class YandexAuthTestMiddleware(YandexAuthMiddleware):
    '''Мидлварина для тестов, которая выставляет тестовых
    yauser и client_application
    '''

    def assign_yauser(self, request):
        request.__class__.yauser = YandexTestUserDescriptor()

    def assign_client_application(self, request):
        request.__class__.client_application = TestApplicationDescriptor()
