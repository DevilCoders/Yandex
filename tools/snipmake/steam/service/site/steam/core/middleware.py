# -*- coding: utf-8 -*-

import urllib

from django.conf import settings
from django.core.exceptions import ObjectDoesNotExist
from django.core.urlresolvers import reverse
from django.http import HttpResponse
from django.shortcuts import redirect
from django.utils import timezone

from blackbox import BlackboxError
from django_yauth.middleware import YandexAuthMiddleware
from django_yauth.user import (YandexUserDescriptor, YandexUser,
                               DjangoUserDescriptor, get_blackbox,
                               profile_should_be_created)
from django_yauth.util import (current_host, get_real_ip)

from ui.settings import SESSIONID_COOKIE_NAME, USER_AUTH_REQUIRED
from core.actions.healthchecker import check_database, SteamHealthError
from core.actions.mailsender import email_address
from core.hard.loghandlers import SteamLogger
from core.models import User


ALIVE_URLS = ('/alive', '/alive/', '/health', '/health/')
YAUTH_URLS = ('/yauth', '/yauth/')


class HTTPSMiddleware:
    SESSIONID_2_COOKIE_NAME = 'sessionid2'

    def process_request(self, request):
        if not USER_AUTH_REQUIRED or request.path in ALIVE_URLS:
            return
        ajax_request = request.path.startswith('/ajax/')
        bb = get_blackbox()
        user_ip = get_real_ip(request)
        try:
            response = bb._blackbox_xml_call(
                method='sessionid',
                sessionid=request.COOKIES.get(SESSIONID_COOKIE_NAME),
                userip=user_ip,
                host=current_host(request),
                sslsessionid=request.COOKIES.get(
                    HTTPSMiddleware.SESSIONID_2_COOKIE_NAME
                ),
                renew='yes'
            )
        except BlackboxError as err:
            SteamLogger.warning(
                'User %(user)s (IP: %(user_ip)s) could not be checked: BlackboxError %(err)s',
                type='PASSPORT_EVENT',
                user=request.yauser.login, user_ip=user_ip, err=err,
            )
            return HttpResponse('Service Temporarily Unavailable', status=503)
        bb_response = response[0]
        auth_node = bb_response.find('auth')
        if not (
            auth_node is not None and
            auth_node.find('secure') is not None and
            auth_node.find('secure').text == '1'
        ):
            if request.path in YAUTH_URLS:
                SteamLogger.info(
                    'User %(user)s (IP: %(user_ip)s) came to YAuth page',
                    type='PASSPORT_EVENT',
                    user=request.yauser.login, user_ip=user_ip,
                )
                return
            if auth_node is None:
                redir_reason = 'no "auth" node'
            elif auth_node.find('secure') is None:
                redir_reason = 'no "auth/secure" node'
            else:
                redir_reason = '"auth/secure" is 0'
            action = 'given ajax 403' if ajax_request else 'redirected'
            SteamLogger.info(
                'User %(user)s (IP: %(user_ip)s) was %(action)s because of: %(reason)s',
                type='PASSPORT_EVENT',
                user=request.yauser.login, user_ip=user_ip,
                action=action,
                reason=redir_reason
            )
            if ajax_request:
                return HttpResponse('', status=403)
            redir = redirect('core:yauth')
            redir['Location'] += '?retpath=%s' % urllib.quote(request.path)
            return redir
        request.bb_response = bb_response

    def process_response(self, request, response):
        if not (
            USER_AUTH_REQUIRED and
            request.path not in ALIVE_URLS and
            hasattr(request, 'bb_response')
        ):
            return response
        domain = '.yandex-team.ru'
        new_sslsession = request.bb_response.find('new-sslsession')
        if new_sslsession is not None:
            sessionid_2 = new_sslsession.text
            expires = int(new_sslsession.get('expires', '0'))
            if expires:
                new_session_expires = timezone.datetime.fromtimestamp(expires)
            else:
                new_session_expires = None
            domain = new_sslsession.attrib.get('domain', domain)
            response.set_cookie(
                HTTPSMiddleware.SESSIONID_2_COOKIE_NAME,  # key
                sessionid_2,                              # value
                None,                                     # max_age
                new_session_expires,                      # expires
                '/',                                      # path
                domain,                                   # domain
                True,                                     # secure
                True                                      # HttpOnly
            )
        return response


class DBAliveMiddleware:
    def process_request(self, request):
        if request.path in ALIVE_URLS[2:]:
            try:
                check_database()
            except SteamHealthError as ex:
                return HttpResponse(str(ex), status=503)

    def process_response(self, request, response):
        return response


class SteamAuthMiddleware(YandexAuthMiddleware):
    def process_request(self, request):
        if USER_AUTH_REQUIRED:
            return YandexAuthMiddleware.process_request(self, request)

        request.__class__.yauser = NonAnonymousYandexUserDescriptor(
            request.COOKIES.get('yandex_login'),
            get_real_ip(request),
            current_host(request))

        if settings.YAUSER_ADMIN_LOGIN:
            request.__class__.user = DjangoUserDescriptor()

        if (
            settings.YAUTH_CREATION_REDIRECT and
            request.method in settings.YAUTH_CREATION_REDIRECT_METHODS and
            request.path != reverse(settings.YAUTH_CREATE_PROFILE_VIEW) and
            profile_should_be_created(request)
        ):
            return self.creation_redirect(request)

    def process_response(self, request, response):
        if USER_AUTH_REQUIRED:
            return YandexAuthMiddleware.process_response(self, request,
                                                         response)
        return response


class NonAnonymousYandexUserDescriptor(YandexUserDescriptor):
    def _get_yandex_user(self):
        try:
            user = User.objects.get(login=self.cookie)
        except ObjectDoesNotExist:
            raise ObjectDoesNotExist(
                'Cookie "yandex_login" value "%s" is not correct' % self.cookie
            )
        session = {
            'status': 'VALID',
            'domain': None,
            'bruteforce_policy': {
                'captcha': False,
                'level': None,
                'password_expired': False
            },
            'login_status': None,
            'uid': user.yandex_uid,
            'oauth': None,
            'default_email': email_address(user.login),
            'lite_uid': None,
            'redirect': False,
            'url': 'http://blackbox.yandex-team.ru/blackbox/',
            'fields': {
                'login': user.login,
                'display_name': user.login,
                'social_aliases': None,
                'social': None
            },
            'age': 65536,
            'password_status': None,
            'emails': [{
                'default': True,
                'validated': True,
                'native': True,
                'address': email_address(user.login)
            }],
            'valid': True,
            'new_session': None,
            'karma': '0',
            'error': 'OK'
        }
        return YandexUser(
            int(session['uid']),
            bool(session['lite_uid']),
            session['fields'],
            session['redirect'],
            session['emails'],
            session['default_email'],
            session['new_session'],
            session,
        )
