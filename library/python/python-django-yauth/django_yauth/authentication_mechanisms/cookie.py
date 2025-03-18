# coding: utf-8

from __future__ import unicode_literals

import logging
import six

if six.PY2:
    from urlparse import urlparse
elif six.PY3:
    from urllib.parse import urlparse

import blackbox
from django.conf import settings

from .base import BaseMechanism
from django_yauth.util import get_setting, generate_tvm_ts_sign
from django_yauth.exceptions import InvalidProtocol, TwoCookiesRequired
from django_yauth.user import YandexUser


log = logging.getLogger(__name__)


class BlackBoxMixin(object):
    @property
    def blackbox_kwargs(self):
        base_kwargs = {
            'url': blackbox.BLACKBOX_URL,
        }
        if settings.YAUTH_USE_TVM2_FOR_BLACKBOX:
            blackbox_host = urlparse(blackbox.BLACKBOX_URL).netloc
            blackbox_client = getattr(settings, 'YAUTH_TVM2_BLACKBOX_CLIENT') or settings.YAUTH_TVM2_BLACKBOX_MAP[
                blackbox_host]

            base_kwargs['blackbox_client'] = blackbox_client
            base_kwargs['tvm2_client_id'] = settings.YAUTH_TVM2_CLIENT_ID
            base_kwargs['tvm2_secret'] = settings.YAUTH_TVM2_SECRET
        return base_kwargs

    @property
    def blackbox(self):
        return getattr(settings, 'YAUTH_BLACKBOX_INSTANCE') or blackbox.XmlBlackbox(**self.blackbox_kwargs)

    @property
    def bb_fields(self):
        return list(get_setting(['PASSPORT_FIELDS', 'YAUTH_PASSPORT_FIELDS']))

    @property
    def bb_params(self):
        kwargs = settings.YAUTH_BLACKBOX_PARAMS.copy()
        if settings.YAUTH_TVM_CLIENT_ID:

            ts, ts_sign = generate_tvm_ts_sign(settings.YAUTH_TVM_CLIENT_SECRET)

            kwargs['getticket'] = 'yes'
            kwargs['client_id'] = settings.YAUTH_TVM_CLIENT_ID
            kwargs['consumer'] = settings.YAUTH_BLACKBOX_CONSUMER
            kwargs['ts'] = ts
            kwargs['ts_sign'] = ts_sign

        elif settings.YAUTH_TVM2_GET_USER_TICKET and settings.YAUTH_USE_TVM2_FOR_BLACKBOX:
            kwargs['get_user_ticket'] = 'yes'

        return kwargs


class BlackboxBasedMechanism(BlackBoxMixin, object):
    session = None

    YandexUser = YandexUser

    def user_by_session(self, session):
        if not session.valid:
            return self.anonymous(session)

        return self.YandexUser(
            uid=int(session.uid or session.lite_uid),
            is_lite=bool(session.lite_uid),
            fields=session.fields,
            need_reset=session.redirect,
            emails=session.emails,
            default_email=session.default_email,
            new_session=session.new_session,
            oauth=session.oauth,
            auth_secure=session.secure,
            new_sslsession=session.new_sslsession,
            ticket=getattr(session, 'ticket', None),
            blackbox_result=session,
            mechanism=self,
            raw_user_ticket=getattr(session, 'user_ticket', None),
        )

    def anonymous(self, session=None):
        """
        Этот метод для совместимости с yauth.
        Если вы используете только django AUTHENTICATION_BACKENDS
        вам достаточно возвращать None вместо AnonymousUser.
        Бекэнды yauth про это знают и будут отрабатывать как нужно.
        """
        return self.AnonymousYandexUser(blackbox_result=session, mechanism=self)


class Mechanism(BaseMechanism, BlackboxBasedMechanism):

    def extract_params(self, request):
        session_id = self._get_session_id(request)
        if session_id is None:
            return None

        return dict(
            session_id=session_id,
            sessionid2=self._get_sessionid2(request),
            sessguard=self._get_sessguard(request),
            server_host=self.get_current_host(request),
            userip=self.get_user_ip(request),
            blackbox_params=self.bb_params,
            bb_fields=self.bb_fields,
            sessionid2_required=settings.YAUTH_SESSIONID2_REQUIRED,
        )

    def _get_session_id(self, request):
        return request.COOKIES.get(get_setting(['SESSIONID_COOKIE_NAME', 'YAUTH_SESSIONID_COOKIE_NAME']))

    def _get_sessionid2(self, request):
        if not settings.YAUTH_SESSIONID2_REQUIRED:
            return

        if not request.is_secure():
            log.error('Authorization error: HTTPS connection is required when using "sessionid2"')
            raise InvalidProtocol('Authorization error: HTTPS connection is required')

        sessionid2 = request.COOKIES.get(settings.YAUTH_SESSIONID2_COOKIE_NAME)

        if not sessionid2 and settings.YAUTH_SESSIONID2_REQUIRED:
            log.warning('Authorization error: no sessionid2 cookie, but is mandatory')
            raise TwoCookiesRequired()

        return request.COOKIES.get(settings.YAUTH_SESSIONID2_COOKIE_NAME)

    def _get_sessguard(self, request):
        return request.COOKIES.get(settings.YAUTH_SESSGUARD_COOKIE_NAME)

    def apply(self, session_id, sessionid2, sessguard, server_host, userip, blackbox_params, bb_fields,
              sessionid2_required):
        blackbox_instance = self.blackbox
        if sessguard is not None:
            blackbox_params.update(sessguard=sessguard)
        try:
            session = blackbox_instance.sessionid(
                session_id, userip, server_host,
                dbfields=bb_fields,
                sslsessionid=sessionid2,
                **blackbox_params
            )

            if 'error' in session and session.error != 'OK':
                log.warning('Blackbox responded with error: %s. Request was from ip %s, host %s.',
                            session.error, userip, server_host)
        except blackbox.BlackboxError:  # если blackbox недоступен, то пользователь анонимен
            log.warning('Blackbox unavailable', exc_info=True)
            return self.AnonymousYandexUser(blackbox_result=None, mechanism=self)

        session.url = blackbox_instance.url

        if sessionid2_required and not session.secure:
            # наличие второй куки является обязательным, но она невалидна
            raise TwoCookiesRequired('session.secure is False')

        if not session.valid and session.secure:
            # хотя кука Session_id невалидна, но вторая кука sessionid2 валидна и соответствует первой
            log.warning('Authorization error: sessionid2 cookie is valid, but Session_id is not')
            raise TwoCookiesRequired()

        return self.user_by_session(session)
