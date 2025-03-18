# coding: utf-8

from __future__ import unicode_literals

import re
import logging

import blackbox
import six
from django.conf import settings
from django.contrib.auth.models import AnonymousUser
from django.core.exceptions import PermissionDenied

from django_yauth.exceptions import AuthException
from .cookie import BlackboxBasedMechanism
from .base import BaseMechanism


log = logging.getLogger(__name__)


class OauthYandexUser(BlackboxBasedMechanism.YandexUser):
    @property
    def scopes(self):
        if self.blackbox_result:
            return set(self.blackbox_result.oauth.scope.split(' '))
        else:
            # сперва нужно сходить в блекбокс
            raise AuthException('You probably called it before you actually got authentication information')

    def check_scopes(self, *required_scopes):
        """
        Проверить что пользователь удовлетворяет скоупам.

        В продакшен можно делать так:

        if yauser.authenticated_by.mechanism_name == 'oauth':
            if yauser.check_scopes('fly_japan', 'bow_to_statue'):
                start_sacred_journey(yauser, 'hidden temple')
            else:
                sorry_bro(yauser)

        @rtype: bool
        """
        return set(required_scopes).issubset(self.scopes)


class Mechanism(BaseMechanism, BlackboxBasedMechanism):
    YandexUser = OauthYandexUser
    session = None

    def extract_params(self, request):
        oauth_token = self._get_oauth_token(request)
        if oauth_token is None:
            return None
        return dict(
            oauth_token=oauth_token,
            server_host=self.get_current_host(request),
            userip=self.get_user_ip(request),
            blackbox_params=self.bb_params,
            bb_fields=self.bb_fields,
        )

    def _get_oauth_token(self, request):
        if not settings.YAUTH_USE_OAUTH:
            return None

        auth = request.META.get('HTTP_AUTHORIZATION')
        if not isinstance(auth, six.string_types):
            return None

        match = re.match(r"(?:%s) (?P<token>.+)" %
                         settings.YAUTH_OAUTH_HEADER_PATTERN_PREFIX, auth)
        if not match:
            return None

        return match.group('token')

    def has_perm(self, user_obj, perm, obj=None):
        """
        Для совместимости с аутентификационными бекэндами джанго.

        @param perm: oauth scope
        """
        if isinstance(user_obj, (self.AnonymousYandexUser, AnonymousUser)):
            raise PermissionDenied('no anonymous access')

        if isinstance(user_obj, OauthYandexUser):
            result = user_obj.check_scopes(perm)
            if not result:
                raise PermissionDenied('allowed scopes are "%s"' % ','.join(user_obj.scopes))
            return True
        return False

    def user_by_session(self, session):
        user = super(Mechanism, self).user_by_session(session)

        if isinstance(user, self.AnonymousYandexUser):
            return user

        if settings.YAUTH_OAUTH_AUTHORIZATION_SCOPES:
            # проверка на особый скоуп, "доступ ко всему приложению".
            # Если у тебя нет этого скоупа то значит, что ты ничего не можешь сделать.
            # Поэтому решили что здесь будем считать его неаутентифицированным.

            if not user.scopes & set(settings.YAUTH_OAUTH_AUTHORIZATION_SCOPES):
                return self.anonymous(session)

        return user

    def apply(self, oauth_token, server_host, userip, blackbox_params, bb_fields):
        blackbox_instance = self.blackbox
        try:
            session = blackbox_instance.oauth(
                oauth_token, userip,
                by_token=True,
                dbfields=bb_fields,
                **blackbox_params
            )

            if 'error' in session and session.error != 'OK':
                log.warning('Blackbox responded with error: %s. Request was from ip %s, host %s.',
                            session.error, userip, server_host)
        except blackbox.BlackboxError:  # если blackbox недоступен, то пользователь анонимен
            log.warning('Blackbox unavailable', exc_info=True)
            return self.AnonymousYandexUser(blackbox_result=None, mechanism=self)

        session.url = blackbox_instance.url

        return self.user_by_session(session)
