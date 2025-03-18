# encoding: utf-8
from __future__ import print_function, unicode_literals

import json
import logging

import requests

from .exceptions import (
    InvalidParamsError,
    TemporaryError,
    TransportError,
    AccessDenied,
    UnknownError,
    ResponseError,
    HTTPError,
    ConnectionError,
    FieldRequiredError,
    BlackboxError
)
from .utils import choose_first_not_none, requests_retry_session
from .environment import URL

log = logging.getLogger(__name__)


class BlackboxErrors(object):
    OK = 0
    INVALID_PARAMS = 2
    DB_FETCHFAILED = 9
    DB_EXCEPTION = 10
    ACCESS_DENIED = 21
    UNKNOWN = 1

    @classmethod
    def get_exception(cls, exc_id):
        mapper = {
            cls.INVALID_PARAMS: InvalidParamsError,
            cls.DB_FETCHFAILED: TemporaryError,
            cls.DB_EXCEPTION: TemporaryError,
            cls.ACCESS_DENIED: AccessDenied,
            cls.UNKNOWN: UnknownError
        }

        return mapper.get(exc_id, ResponseError)


class BlackboxDBFields(object):
    LOGIN = 'accounts.login.uid'
    FIO = 'account_info.fio.uid'
    NICKNAME = 'account_info.nickname.uid'
    SEX = 'account_info.sex.uid'
    EMAIL = 'account_info.email.uid'
    COUNTRY = 'userinfo.country.uid'
    LANGUAGE = 'userinfo.lang.uid'
    SUID = 'subscription.suid.%d'
    LOGIN_RULE = 'subscription.login_rule.33'


class BlackboxMixin(object):
    @staticmethod
    def _prepare_payload(payload):
        dbfields = payload.get('dbfields')

        if dbfields and isinstance(dbfields, (list, tuple)):
            payload['dbfields'] = ','.join(dbfields)

        userip = payload.get('userip')
        if userip and userip.startswith('::ffff:'):
            payload['userip'] = userip[len('::ffff:'):]

        return payload

    @staticmethod
    def _parse_response(raw_response):
        return json.loads(raw_response)

    @staticmethod
    def _raise_error(parsed):
        exc = parsed.get('exception')

        if not exc:
            return

        err = parsed['error']
        exception = BlackboxErrors.get_exception(exc['id'])

        log.debug('Get error from blackbox: <%s>', parsed)

        raise exception(exc['id'], exc['value'], err)


class Blackbox(BlackboxMixin):
    """ Черный ящик — это внутренний HTTP-сервис,
    позволяющий публичным сервисам Яндекса проверять авторизационные данные пользователей.
    также Черный ящик дает возможность получать информацию о пользователях из базы данных Паспорта.

    подробнее: https://doc.yandex-team.ru/blackbox
    """

    def __init__(self, url=URL, timeout=1, retries=3, session=None, **kwargs):
        """ Клиент для доступа к blackbox


        :param str url: url до ЧЯ, по умолчанию урл определяется yenv
        :param int timeout: таймаут на чтение и запись
        :param int retries: количество повторных обращений при возникновении ошибок
        :param requests.Session session: сессия для доступа к ЧЯ
        """
        self.url = url
        self.timeout = timeout
        self.retries = retries
        self.session = session or requests.Session()

        self.backoff_factor = kwargs.get('backoff_factor', 0.3)
        self.status_forcelist = kwargs.get('status_forcelist', (500, 502, 504))
        self.format = kwargs.get('format', 'json')

        if self.retries:
            self.session = requests_retry_session(
                self.retries, self.backoff_factor, self.status_forcelist, self.session
            )

    def _do_req(self, http_method, **kwargs):
        """

        :param str|unicode http_method:
        :rtype: requests.Response
        """
        try:
            return self.session.request(http_method, self.url, timeout=self.timeout, **kwargs)
        except (requests.ConnectionError, requests.Timeout) as e:
            raise ConnectionError(e)
        except requests.RequestException as e:
            raise TransportError(e)

    def _req(self, http_method, **kwargs):
        """

        :param str|unicode http_method:
        :param kwargs:
        :rtype dict:
        """
        response = self._do_req(http_method, **kwargs)

        try:
            response.raise_for_status()
        except requests.HTTPError as e:
            raise HTTPError(e, response)

        parsed = self._parse_response(response.text)

        try:
            self._raise_error(parsed)
        except TemporaryError:
            # TODO сделать опциональный перезапуск
            raise

        return parsed

    def _get(self, blackbox_method, required_fields=None, exclusive_fields=None, headers=None, **extra):
        return self.custom_method('GET', blackbox_method, required_fields, exclusive_fields, headers, **extra)

    def _post(self, blackbox_method, required_fields=None, exclusive_fields=None, headers=None, **extra):
        return self.custom_method('POST', blackbox_method, required_fields, exclusive_fields, headers, **extra)

    def custom_method(self, http_method, blackbox_method, required_fields=None, exclusive_fields=None, headers=None, **extra):
        """ Вызов метода `blackbox_method` в ЧЯ

        если нужного метода в клиенте еще нет
        можно легко реализовать свой метод

        :param str|unicode http_method: HTTP метод для вызова (GET, POST)
        :param str|unicode blackbox_method: метод в blackbox (прим. userinfo)
        :param required_fields: обязательные параметры метода
        :param exclusive_fields: обязательные взаимоисключающие параметры
        :param extra: необязательные параметры
        :rtype: dict
        """
        payload = {
            'method': blackbox_method,
            'format': self.format
        }

        payload.update(required_fields)

        if exclusive_fields:
            exclusive = choose_first_not_none(exclusive_fields)
            if not exclusive:
                raise FieldRequiredError("choose from %s" % exclusive_fields.keys())
            payload.update(exclusive)

        payload.update(extra)

        self._prepare_payload(payload)

        return self._make_request(http_method, payload, headers=(headers or {}))

    def _make_request(self, http_method, payload, **kwargs):
        """

        :param str|unicode http_method:
        :param dict payload:
        :param kwargs:
        :rtype dict:
        """
        if http_method == 'GET':
            return self._req('GET', params=payload, **kwargs)
        elif http_method == 'POST':
            return self._req('POST', data=payload, **kwargs)
        else:
            raise BlackboxError("Unknown http method %s" % http_method)

    def userinfo(self, userip, uid=None, suid=None, login=None, headers=None, **extra):
        """ Метод userinfo возвращает сведения о пользователе.

        Основное назначение метода — доступ к данным системы авторизации

        https://doc.yandex-team.ru/blackbox/reference/MethodUserInfo.xml

        обязательные параметры:
        :param str userip:

        взаимоисключающие параметры
        :param str uid:
        :param str suid:
        :param str login:

        :param extra: дополнительные параметры
        :rtype: dict
        """
        method_name = 'userinfo'

        required = {
            'userip': userip,
        }

        exclusive = {
            'uid': uid,
            'suid': suid,
            'login': login
        }

        return self._get(method_name, required, exclusive, headers, **extra)

    def login(self, userip, password, authtype, uid=None, login=None, headers=None, **extra):
        """Метод login проверяет пароль для учетной записи, идентифицированной логином или UID.

        При успешной аутентификации метод также может возвращать сведения об учетной записи,
        запрошенные в базе данных Паспорта

        https://doc.yandex-team.ru/blackbox/reference/MethodLogin.xml

        обязательные параметры:
        :param str userip:
        :param str password:
        :param str authtype:

        взаимоисключающие параметры
        :param str uid:
        :param str login:

        :param extra: дополнительные параметры
        :rtype: dict
        """
        method_name = 'login'

        required = {
            'userip': userip,
            'password': password,
            'authtype': authtype
        }

        exclusive = {
            'uid': uid,
            'login': login
        }

        return self._post(method_name, required, exclusive, headers, **extra)

    def sessionid(self, userip, sessionid, host, headers=None, **extra):
        """Метод sessionid проверяет валидность кук Session_id и sessionid2.

        При успешной аутентификации метод также может возвращать сведения
        о соответствующей учетной записи из базы данных Паспорта

        https://doc.yandex-team.ru/blackbox/reference/MethodSessionID.xml

        обязательные параметры
        :param str userip:
        :param str sessionid:
        :param str host:

        :param extra: дополнительные параметры
        :rtype: dict
        """
        method_name = 'sessionid'

        required = {
            'sessionid': sessionid,
            'userip': userip,
            'host': host
        }

        return self._get(method_name, required, headers=headers, **extra)

    def oauth(self, userip, oauth_token, headers=None, **extra):
        """ Метод oauth проверяет OAuth-токен, выданный сервисом oauth.yandex.ru.

        Если токен валиден, метод также может возвращать запрошенные сведения
        о соответствующей учетной записи из базы данных Паспорта

        https://doc.yandex-team.ru/blackbox/reference/method-oauth.xml

        обязательные параметры
        :param userip:
        :param oauth_token:

        :param extra: дополнительные параметры
        :rtype: dict
        """
        method_name = 'oauth'

        required = {
            'userip': userip,
        }

        headers = dict(headers) if headers else {}
        headers["Authorization"] = "OAuth {}".format(oauth_token)

        return self._get(method_name, required, headers=headers, **extra)

    def lcookie(self, l, headers=None, **extra):
        """ Метод lcookie извлекает данные из переданной куки L: UID, логин и время выставления куки.

        Чтобы вызывать метод, необходимо запросить грант allow_l_cookie в рассылке passport-admin@.

        https://doc.yandex-team.ru/blackbox/reference/method-lcookie.xml

        обязательные параметры
        :param str|unicode l:

        :param extra:
        :rtype: dict
        """
        method_name = 'lcookie'

        required = {
            'l': l
        }

        return self._get(method_name, required, headers=headers, **extra)

    def checkip(self, ip, nets, headers=None, **extra):
        """ Метод checkip проверяет, принадлежит ли IP-адрес пользователя сетям Яндекса.

        https://doc.yandex-team.ru/blackbox/reference/method-checkip.xml

        обязательные параметры
        :param str|unicode ip:
        :param str|unicode nets:

        :param extra:
        :rtype: dict
        """
        method_name = 'checkip'

        required = {
            'ip': ip,
            'nets': nets
        }

        return self._get(method_name, required, headers=headers)
