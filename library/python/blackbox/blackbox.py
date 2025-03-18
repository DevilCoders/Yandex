# -*- coding: utf-8 -*-
import logging
import time
import os
import socket
import warnings
from email.utils import formatdate
from functools import wraps

import xml.etree.ElementTree as et
from retrying import retry

import six
from six.moves import urllib, http_client

import yenv

# Нужно чтобы работали соединения с таймаутами
try:
    from httplib2 import socks
except ImportError:
    socks = None

try:
    import ssl  # python 2.6

    _ssl_wrap_socket = ssl.wrap_socket
except (AttributeError, ImportError):
    def _ssl_wrap_socket(sock, key_file, cert_file):
        ssl_sock = socket.ssl(sock, key_file, cert_file)
        return http_client.FakeSocket(sock, ssl_sock)


# Следующие два класса взяты с небольшими изменениями из httplib2.

class ProxiesUnavailableError(Exception):
    pass


class HTTPConnectionWithTimeout(http_client.HTTPConnection):
    """HTTPConnection subclass that supports timeouts

    All timeouts are in seconds. If None is passed for timeout then
    Python's default timeout for sockets will be used. See for example
    the docs of socket.setdefaulttimeout():
    http://docs.python.org/library/socket.html#socket.setdefaulttimeout

    """

    def __init__(self, host, port=None, strict=None, timeout=None, proxy_info=None):
        http_client.HTTPConnection.__init__(self, host, port, strict)
        self.proxy_info = proxy_info
        self.timeout = timeout or HTTP_TIMEOUT

    def connect(self):
        """Connect to the host and port specified in __init__."""

        # Mostly verbatim from httplib.py.
        if self.proxy_info and socks is None:
            raise ProxiesUnavailableError(
                'Proxy support missing but proxy use was requested!')
        msg = "getaddrinfo returns an empty list"
        for res in socket.getaddrinfo(self.host, self.port, 0,
                                      socket.SOCK_STREAM):
            af, socktype, proto, canonname, sa = res
            try:
                if self.proxy_info and self.proxy_info.isgood():
                    self.sock = socks.socksocket(af, socktype, proto)
                    self.sock.setproxy(*self.proxy_info.astuple())
                else:
                    self.sock = socket.socket(af, socktype, proto)
                    self.sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
                # Different from httplib: support timeouts.
                self.sock.settimeout(self.timeout)
                # End of difference from httplib.
                if self.debuglevel > 0:
                    print("connect: (%s, %s)" % (self.host, self.port))

                self.sock.connect(sa)
            except socket.error as err:
                msg = err
                if self.debuglevel > 0:
                    print('connect fail:', (self.host, self.port))
                if self.sock:
                    self.sock.close()
                self.sock = None
                continue
            break
        if not self.sock:
            raise socket.error(msg)


class HTTPSConnectionWithTimeout(http_client.HTTPSConnection):
    """This class allows communication via SSL.

    All timeouts are in seconds. If None is passed for timeout then
    Python's default timeout for sockets will be used. See for example
    the docs of socket.setdefaulttimeout():
    http://docs.python.org/library/socket.html#socket.setdefaulttimeout

    """

    def __init__(self, host, port=None, key_file=None, cert_file=None,
                 strict=None, timeout=None, proxy_info=None):
        kwargs = {}
        if six.PY2:
            kwargs.update(strict=strict)
        http_client.HTTPSConnection.__init__(self, host, port=port, key_file=key_file,
                                             cert_file=cert_file, **kwargs)
        self.proxy_info = proxy_info
        self.timeout = timeout or HTTP_TIMEOUT

    def connect(self):
        """Connect to a host on a given (SSL) port."""

        msg = "getaddrinfo returns an empty list"
        for family, socktype, proto, canonname, sockaddr in socket.getaddrinfo(
                self.host, self.port, 0, socket.SOCK_STREAM):
            try:
                if self.proxy_info and self.proxy_info.isgood():
                    sock = socks.socksocket(family, socktype, proto)
                    sock.setproxy(*self.proxy_info.astuple())
                else:
                    sock = socket.socket(family, socktype, proto)
                    sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)

                sock.settimeout(self.timeout)
                sock.connect((self.host, self.port))
                self.sock = _ssl_wrap_socket(sock, self.key_file, self.cert_file)
                if self.debuglevel > 0:
                    print("connect: (%s, %s)" % (self.host, self.port))
            except socket.error as err:
                msg = err
                if self.debuglevel > 0:
                    print('connect fail:', (self.host, self.port))
                if self.sock:
                    self.sock.close()
                self.sock = None
                continue
            break
        if not self.sock:
            raise socket.error(msg)


def smart_str(s):
    if not isinstance(s, six.string_types):
        return str(s)
    elif isinstance(s, six.text_type):
        return s.encode('utf-8', 'replace')
    else:
        return s


def can_retry(exc):
    return (isinstance(exc, BlackboxResponseError)
            and 'DB_EXCEPTION' in str(exc))


# Взято из Django
def urlencode(query, doseq=0):
    """
    A version of Python's urllib.urlencode() function that can operate on
    unicode strings. The parameters are first case to UTF-8 encoded strings and
    then encoded as per normal.

    """
    if hasattr(query, 'items'):
        query = list(query.items())
    return urllib.parse.urlencode(
        [(smart_str(k),
          isinstance(v, (list, tuple)) and [smart_str(i) for i in v] or smart_str(v))
         for k, v in query],
        doseq)


TVM2_RETRY_ATTEMPT = 3
TVM2_SERVICE_TICKET_HEADER = 'X-Ya-Service-Ticket'

log = logging.getLogger(__name__)

FIELD_LOGIN = ('accounts.login.uid', 'login')
FIELD_FIO = ('account_info.fio.uid', 'fio')
FIELD_NICKNAME = ('account_info.nickname.uid', 'nickname')
FIELD_SEX = ('account_info.sex.uid', 'sex')
FIELD_EMAIL = ('account_info.email.uid', 'email')
FIELD_COUNTRY = ('userinfo.country.uid', 'country')
FIELD_LANGUAGE = ('userinfo.lang.uid', 'language')

# нужно для получения типа пользователя
FIELD_LOGIN_RULE = ('subscription.login_rule.33', 'login_rule')

HTTP_HEADERS = {}

BLACKBOX_RETRY_ERRORS_ATTEMPTS = 3


def FIELD_SUID(sid):
    """
    Если пользователь подписан на сервис sid такой-то, в ответе
    будет не-None значение в поле suid.

    """
    return ('subscription.suid.%d' % sid, 'suid')


class BlackboxError(Exception):
    """Base Blackbox error"""


class BlackboxConnectionError(BlackboxError):
    """Error when connect to Blackbox"""


class BlackboxResponseError(BlackboxError):
    """Error in Blackbox response
    Attributes:
        * status - response status
        * content_type - response content type
        * content - response body
    """

    def __init__(self, msg, status=None, content_type=None, content=None, *args):
        super(BlackboxResponseError, self).__init__(msg, *args)
        self.status = status
        self.content_type = content_type
        self.content = content


class odict(dict):
    """Обёртка для словаря, добавляет доступ по именам атрибутов."""

    def __getattr__(self, name):
        try:
            return self[name]
        except KeyError:
            raise AttributeError(name)

    def __setattr__(self, name, value):
        self[name] = value


class BaseBlackbox(object):
    """Базовый класс для обращения к JSON- и XML-API Blackbox'а.

    Документация к Blackbox API:
        http://doc.yandex-team.ru/blackbox/

    """
    TIMEOUT = 2  # Таймаут HTTP-запросов
    RETRY_INTERVAL = 0.5  # Интервал между повторами

    # Мапинг урлов для различных окружений
    URLS = {
        'intranet': {
            'production': 'http://blackbox.yandex-team.ru/blackbox/',
        },
        'other': {
            'development': 'http://blackbox-mimino.yandex.net/blackbox',
            'rc': 'http://blackbox-rc.yandex.net/blackbox',
            'production': 'http://blackbox.yandex.net/blackbox/',
        }
    }

    # В интранете только один ЧЯ
    URLS['intranet']['development'] = URLS['intranet']['production']
    URLS['intranet']['testing'] = URLS['intranet']['production']
    URLS['intranet']['rc'] = URLS['intranet']['production']

    # В тестинге тестовый ЧЯ
    URLS['other']['testing'] = URLS['other']['development']

    # Считаем что на локалхосте есть дырки до обычных инстансов
    URLS['localhost'] = URLS['other']

    # Бета - обычно нужен стандарный паспорт
    URLS['beta'] = URLS['other']

    # Для модели яндекса в масштабе 1:43
    URLS['model143'] = {'production': 'http://blackbox.ya-test.ru/blackbox/'}
    URLS['model143']['development'] = URLS['model143']['production']
    URLS['model143']['testing'] = URLS['model143']['production']

    # Окружение для нагрузочного тестирования
    URLS['stress'] = {'stress': URLS['other']['development']}

    # Автоопределенный url
    try:
        URL = yenv.choose_key_by_type(yenv.choose_key_by_name(URLS, fallback=True), fallback=True)
    except ValueError:
        URL = None

    def __init__(self, url=None, timeout=None, dbfields=None,
                 retry_count=None, tvm2_client_id=None,
                 tvm2_secret=None, blackbox_client=None,
                 force_tvm2_initialization=False):
        """
        :param tvm2_client_id: six.string_types
        :param tvm2_secret: six.string_types
        :param force_tvm2_initialization: bool
        :param blackbox_client: ticket_parser2.api.v1.BlackboxClientId
        """

        self.url = url or self.URL
        if not self.url:
            raise ValueError("No blackbox url specified and none could be derived from environment")
        self.timeout = timeout or self.TIMEOUT
        if hasattr(timeout, '__iter__'):
            self._timeouts = self.timeout
        elif retry_count:
            self._timeouts = [self.timeout for _ in range(retry_count)]
        else:
            self._timeouts = [self.timeout, self.timeout]

        self.dbfields = dbfields
        self.use_tvm2 = False
        if tvm2_client_id and blackbox_client:
            tvm2_use_qloud = os.environ.get('TVM2_USE_QLOUD') or os.environ.get('TVM2_USE_DAEMON')
            if tvm2_use_qloud or tvm2_secret:
                self.use_tvm2 = True
                self.blackbox_client_id = blackbox_client.value
                self.blackbox_client = blackbox_client
                self.tvm2_secret = tvm2_secret
                self.tvm2_client_id = tvm2_client_id

                if force_tvm2_initialization:
                    self._get_tvm2_client()
            else:
                log.warning(
                    "To use TVM2 either define 'tvm2_secret' keyword argument "
                    "or set the environment variable 'TVM2_USE_DAEMON'"
                )

    def _get_tvm2_client(self):
        current_client = getattr(self, '_tvm2_client', None)
        if not current_client:
            from tvm2 import TVM2
            # Importing tvm2 here to make it an optional dependency
            # https://st.yandex-team.ru/DEVTOOLSUP-7654#5c4704212f38ef001f59e1f0
            current_client = TVM2(
                client_id=self.tvm2_client_id,
                blackbox_client=self.blackbox_client,
                secret=self.tvm2_secret,
                destinations=(self.blackbox_client_id,),
            )
            setattr(self, '_tvm2_client', current_client)
        return current_client

    def _blackbox_http_call(self, params, headers=None, timeout=None):
        parts = urllib.parse.urlparse(self.url)

        if parts.scheme == 'http':
            cls = HTTPConnectionWithTimeout
        elif parts.scheme == 'https':
            cls = HTTPSConnectionWithTimeout
        else:
            raise ValueError(
                'Unavailable scheme "%s" in blackbox URL "%s"' %
                (parts.scheme, BLACKBOX_URL)
            )

        default_headers = HTTP_HEADERS.copy()

        if self.use_tvm2:
            tvm2_client = self._get_tvm2_client()

            for _ in range(TVM2_RETRY_ATTEMPT):
                service_tickets = tvm2_client.get_service_tickets(
                    self.blackbox_client_id
                )
                blackbox_ticket = service_tickets.get(self.blackbox_client_id)
                if blackbox_ticket:
                    default_headers[TVM2_SERVICE_TICKET_HEADER] = blackbox_ticket
                    break
            else:
                log.error('Couldn\'t get service ticket for blackbox')

        if params.get('method', '') == 'login':
            httpmethod = 'POST'
            path = parts.path
            body = urlencode(params)
            default_headers['Content-Type'] = 'application/x-www-form-urlencoded'
        else:
            httpmethod = 'GET'
            path = parts.path + '?' + urlencode(params)
            body = None

        if not path.startswith('/'):
            path = '/' + path

        default_headers.update(headers or {})

        try:
            conn = cls(parts.hostname, parts.port, timeout=timeout)
            conn.request(httpmethod, path, body, default_headers)
            kwargs = {}
            if six.PY2:
                kwargs.update(buffering=True)
            resp = conn.getresponse(**kwargs)
        except (socket.error, http_client.HTTPException) as ex:
            raise BlackboxConnectionError(ex)

        response = resp.read()
        content_type = resp.getheader('content-type', 'text/xml').split(';')[0]

        # По документации ЧЯ всегда возвращает 200, поэтому любой другой код
        # считается ошибкой.
        if resp.status != 200:
            raise BlackboxResponseError(
                'Unexpected status code %s' % resp.status,
                status=resp.status,
                content_type=content_type,
                content=response,
            )

        format = params.get('format', 'xml')
        format_content_types = {
            'json': ['application/json'],
            'xml': ['text/xml', 'application/xml'],
        }[format]

        if content_type not in format_content_types:
            raise BlackboxResponseError(
                'Unexpected content type "%s", need %s' % (
                    content_type, ' or '.join(format_content_types)
                ),
                status=resp.status,
                content_type=content_type,
                content=response,
            )

        return response

    def _blackbox_call(self, method, headers=None, **kw):
        """Выполняет http запрос к blackbox."""

        params = dict(kw)
        params['method'] = method

        if 'dbfields' in params:
            params['dbfields'] = ','.join(params['dbfields'])

        ipv6_prefix = '::ffff:'
        userip = params.get('userip', None)
        if userip is not None and userip.startswith(ipv6_prefix):
            params['userip'] = userip[len(ipv6_prefix):]

        timeouts = list(self._timeouts)
        for i, timeout in enumerate(timeouts[:-1]):
            ts = time.time()
            try:
                return self._blackbox_http_call(params, headers, timeout)
            except Exception as e:
                log.debug('_blackbox_http_call(*%s, **%s) try %i failed, retrying',
                          (method, headers), kw, i + 1, exc_info=True)

                request_time = time.time() - ts
                remaining_timeout = timeout - request_time
                if remaining_timeout < 0:
                    remaining_timeout = 0
                time.sleep(remaining_timeout + self.RETRY_INTERVAL)

        return self._blackbox_http_call(params, headers, timeouts[-1])

    def _check_login_rule(self, login_rule):
        """Проверяет тип пользователя по полю login_rule на 33ем сервисе.

        Возвращает True если это лайт-пользователь. Правило:

        login_rule:
        1 – лайт-пользователь, свой пароль;
        2 – лайт-пользователь, авто пароль;
        3 – дорегистрированный пользователь(то есть уже не лайт);
        отсутствие - не лайт.

        """
        if login_rule is None:
            return False

        return int(login_rule) != 3


class XmlBlackbox(BaseBlackbox):

    @retry(retry_on_exception=can_retry, stop_max_attempt_number=BLACKBOX_RETRY_ERRORS_ATTEMPTS)
    def _blackbox_xml_call(self, method, dbfields=None, headers=None, **kw):
        """Выполняет запрос к XML-API.

        Возвращает полный ответ в виде ElementTree.Element и, дополнительно,
        dictionary выбранных из ответа общих атрибутов (status, error, uid,
        dbfields).
        В случае blackbox exception выкидывается BlackboxError.

        @param dbfields список констант FIELD_xxx

        """
        dbfields = dict(dbfields) if dbfields else {}
        raw_dbfields = list(dbfields.keys())

        xml_response = self._blackbox_call(method, headers=headers,
                                           dbfields=raw_dbfields, **kw)
        root = et.XML(xml_response)
        return root, self._parse_attrs(root, dbfields)

    def _parse_attrs(self, root, dbfields):
        error = root.findtext('error')
        exception = root.findtext('exception', None)
        if exception:
            raise BlackboxResponseError('%s: %s' % (exception, error))

        status = root.findtext('status')

        uid_node = root.find('uid')
        if uid_node is None:
            domain = None
            uid = None
        else:
            hosted = uid_node.get('hosted') == '1'
            domain = uid_node.get('domain') if hosted else None

            # uid_node.text gives None in case of empty <uid/>.
            # We need an empty string for backwards compatibility
            uid = uid_node.text or ''

        lite_uid = root.findtext('liteuid', None)

        try:
            node = root.find('karma')
            confirmed = int(node.attrib.get('confirmed', 0))
            karma = str(int(node.text) + 1000 * confirmed)
        except:
            karma = '0'

        try:
            node = root.find('karma_status')
            karma_status = node.text
        except:
            karma_status = '0'

        fields = odict(
            (dbfields.get(i.attrib['id'], i.attrib['id']), i.text)
            for i in root.findall('dbfield')
        )

        emails = [
            odict(
                default=address.attrib['default'] == '1',
                address=address.text,
                validated=address.attrib['validated'] == '1',
                native=address.attrib['native'] == '1'
            )
            for address in root.findall('address-list/address')
            ]

        social_aliases = [
                             alias.text for alias in root.findall('aliases/alias')
                             if (not alias.text is None) and (alias.attrib['type'] == '6')
                             ] or None

        aliases = [
                      (alias.attrib['type'], alias.text) for alias in root.findall('aliases/alias')
                      if not alias.text is None
                      ] or None

        display_name_xml = root.find('display_name/name')
        if not display_name_xml is None:
            display_name = display_name_xml.text
        else:
            display_name = None

        # Поле avatar пока не описано в официальной документации.
        # См. https://wiki.yandex-team.ru/yapic/#pasportnoeapi
        default_avatar_xml = root.find('display_name/avatar/default')
        if default_avatar_xml is not None:
            default_avatar_id = default_avatar_xml.text
        else:
            default_avatar_id = None

        profile_id_xml = root.find('display_name/social/profile_id')
        provider_xml = root.find('display_name/social/provider')

        if profile_id_xml is not None and provider_xml is not None:
            social = odict(profile_id=profile_id_xml.text,
                           provider=provider_xml.text)
        else:
            social = None

        fields.update(
            social_aliases=social_aliases,
            aliases=aliases,
            display_name=display_name,
            default_avatar_id=default_avatar_id,
            social=social,
        )

        if FIELD_LOGIN[0] in dbfields and fields.get('social_aliases') and not fields.login:
            fields[FIELD_LOGIN[1]] = fields.get('social_aliases')[0]

        default_email = None
        if emails:
            try:
                default_email = [email.address for email in emails if email.default][0]
            except IndexError:
                pass

        oauth_xml = root.find('OAuth')
        if oauth_xml is not None:
            oauth = {}
            for node in oauth_xml:
                oauth[node.tag] = node.text
            oauth = odict(oauth)
        else:
            oauth = None

        ticket_node = root.find('ticket')
        ticket = None if ticket_node is None else ticket_node.text

        user_ticket_node = root.find('user_ticket')
        user_ticket = None if user_ticket_node is None else user_ticket_node.text

        show_captcha_xml = root.find('bruteforce_policy/captcha')
        password_expired_xml = root.find('bruteforce_policy/password_expired')
        bruteforce_policy = odict(captcha=True if show_captcha_xml is not None else False,
                                  level=None,
                                  password_expired=True if password_expired_xml is not None else False,
                                  )
        login_status_xml = root.find('login_status')
        password_status_xml = root.find('password_status')
        connection_id_xml = root.find('connection_id')
        public_id_xml = root.find('public_id')

        login_status = login_status_xml.text if login_status_xml is not None else None
        password_status = password_status_xml.text if password_status_xml is not None else None
        connection_id = connection_id_xml.text if connection_id_xml is not None else None
        public_id = public_id_xml.text if public_id_xml is not None else None

        phones = None
        phones_xml = root.find('phones')
        if phones_xml:
            phones = []
            for phone_xml in phones_xml:
                phone_id = phone_xml.attrib['id']
                phone_attributes = {}
                for phone_attribute_xml in phone_xml:
                    phone_attribute_type = phone_attribute_xml.attrib['type']
                    phone_attribute_value = phone_attribute_xml.text
                    phone_attributes[phone_attribute_type] = phone_attribute_value
                phone_attributes = odict(phone_attributes)
                phone = odict(id=phone_id, attributes=phone_attributes)
                phones.append(phone)

        attributes = None
        attributes_xml = root.find('attributes')
        if attributes_xml:
            attributes = {}
            for attr_xml in attributes_xml:
                attributes[attr_xml.attrib['type']] = attr_xml.text

        return dict(error=error,
                    status=status,
                    uid=uid,
                    oauth=oauth,
                    ticket=ticket,
                    user_ticket=user_ticket,
                    lite_uid=lite_uid,
                    karma=karma,
                    karma_status=karma_status,
                    domain=domain,
                    fields=fields,
                    emails=emails or None,
                    bruteforce_policy=bruteforce_policy,
                    default_email=default_email,
                    login_status=login_status,
                    password_status=password_status,
                    connection_id=connection_id,
                    phones=phones,
                    attributes=attributes,
                    public_id=public_id,
                    )

    def _social_params(self, dbfields, social_info, kw):
        if (dbfields and FIELD_LOGIN in dbfields) or social_info:
            kw['regname'] = 'yes'
            kw['aliases'] = 'all'

    def _parse_new_session_elm(self, new_session_node):
        new_session = None
        if new_session_node is not None:
            if new_session_node.get('expires') != '0':
                rfcdate = formatdate(int(new_session_node.get('expires')))
                new_session_expires = '%s-%s-%s GMT' % (
                rfcdate[:7], rfcdate[8:11], rfcdate[12:25])  # taken from Django
            else:
                new_session_expires = None

            new_session = odict(
                id=new_session_node.text,
                domain=new_session_node.get('domain'),
                expires=new_session_expires,
                http_only=bool(new_session_node.get('HttpOnly'))
            )

        return new_session

    def sessionid(self, sessionid, userip, host, dbfields=None, social_info=True, **kw):
        """blackbox.sessionid -- проверка валидности кук Session_id и sessionid2."""

        self._social_params(dbfields, social_info, kw)

        root, attrs = self._blackbox_xml_call('sessionid',
                                              dbfields or self.dbfields,
                                              sessionid=sessionid,
                                              userip=userip,
                                              host=host,
                                              **kw)
        status = attrs['status']

        # sessionid custom attrs
        try:
            age = int(root.findtext('age'))
        except (TypeError, ValueError):
            age = None

        valid = (status == 'VALID' or status == 'NEED_RESET')
        redirect = status == 'NEED_RESET'
        secure = root.findtext('./auth/secure') == '1'
        password_verification_age = root.findtext('./auth/password_verification_age')
        if password_verification_age is not None:
            password_verification_age = int(password_verification_age)

        new_session = self._parse_new_session_elm(root.find('new-session'))
        new_sslsession = self._parse_new_session_elm(root.find('new-sslsession'))

        attrs.update(age=age, valid=valid, redirect=redirect, new_session=new_session,
                     password_verification_age=password_verification_age,
                     secure=secure, new_sslsession=new_sslsession)
        return odict(**attrs)

    def login(self, login, password, userip, dbfields=None, social_info=True, **kw):
        """blackbox.login -- проверка логина-пароля."""

        self._social_params(dbfields, social_info, kw)

        root, attrs = self._blackbox_xml_call('login',
                                              dbfields or self.dbfields,
                                              login=login,
                                              password=password,
                                              userip=userip,
                                              **kw)

        return odict(**attrs)

    def oauth(self, headers_or_token, userip, dbfields=None, by_token=False, **kw):
        if by_token:
            root, attrs = self._blackbox_xml_call('oauth',
                                                  dbfields,
                                                  oauth_token=headers_or_token,
                                                  userip=userip,
                                                  **kw)
        else:
            root, attrs = self._blackbox_xml_call('oauth',
                                                  dbfields,
                                                  headers=headers_or_token,
                                                  userip=userip,
                                                  **kw)

        return odict(valid=(attrs['status'] == 'VALID'), redirect=False, new_session=None,
                     secure=False, new_sslsession=None, **attrs)

    def userinfo(self, uid_or_login, userip, dbfields=None, by_login=False, social_info=True, **kw):
        """blackbox.userinfo -- сведения о пользователе по его uid или login."""

        kw['login' if by_login else 'uid'] = uid_or_login
        self._social_params(dbfields, social_info, kw)

        root, attrs = self._blackbox_xml_call('userinfo',
                                              dbfields or self.dbfields,
                                              userip=userip,
                                              **kw)
        return odict(**attrs)

    def user_ticket(self, user_ticket, dbfields=None, social_info=True, **kw):
        """blackbox.user_ticket -- сведения о пользователе по его user-ticket.
        
        https://docs.yandex-team.ru/blackbox/methods/user_ticket

        """

        self._social_params(dbfields, social_info, kw)
        root, attrs = self._blackbox_xml_call('user_ticket',
                                              dbfields=dbfields or self.dbfields,
                                              user_ticket=user_ticket,
                                              **kw)

        return odict(**attrs)

    def uid(self, login, **kw):
        """Получить UID по логину пользователя, else None."""

        result = self.userinfo(uid_or_login=login, userip='127.0.0.1',
                               by_login=True, **kw)

        try:
            return int(result['uid'])
        except ValueError:
            return None

    def subscription(self, uid, sid):
        """Проверка подписки пользователя на сервис по uid.

        Выдаёт suid и uid пользователя на сервисе, если у пользователя имеется
        подписка на указанный сервис, иначе None.

        """
        result = self.userinfo(uid_or_login=uid, userip='127.0.0.1',
                               dbfields=[FIELD_SUID(sid), ])

        try:
            suid = result['fields']['suid']
            if suid is None:
                return None
            return dict(uid=result['uid'],
                        suid=result['fields']['suid'])
        except KeyError:
            return None

    def is_email_valid(self, uid, email):
        root, _ = self._blackbox_xml_call('userinfo', dbfields=None, uid=uid,
                                          userip='127.0.0.1', emails='testone',
                                          addrtotest=email)
        node = root.find('address-list/address')
        if node is None:
            return False  # not added

        return node.attrib['validated'] == '1'

    def list_emails(self, uid, only_validated=True):
        root, _ = self._blackbox_xml_call('userinfo', dbfields=None, uid=uid,
                                          userip='127.0.0.1', emails='getall')
        return [el.text for el in root.findall('address-list/address')
                if el.attrib['validated'] or not only_validated]

    def sex(self, uid):
        """Пол пользователя по uid

        значение 0 - пол не указан
        значение 1 - мужской
        значение 2 - женский

        """
        result = self.userinfo(uid_or_login=uid, userip='127.0.0.1',
                               dbfields=[FIELD_SEX, ])

        try:
            if result['fields']['sex'] is None:
                return None

            return int(result['fields']['sex'])
        except (KeyError, ValueError):
            return None

    def is_lite(self, uid):
        """Проверяет что пользователь с данным uid'ом лайт-пользователь."""

        result = self.userinfo(uid_or_login=uid, userip='127.0.0.1',
                               dbfields=[FIELD_LOGIN_RULE])

        return self._check_login_rule(result['fields']['login_rule'])

    def country(self, uid):
        """Страна регистрации пользователя."""

        result = self.userinfo(uid_or_login=uid, userip='127.0.0.1',
                               dbfields=[FIELD_COUNTRY, ])

        try:
            return result['fields']['country']
        except KeyError:
            return None


class JsonBlackbox(BaseBlackbox):
    """Тонкая прослойка над JSON-API Blackbox'а."""

    def __init__(self, *args, **kwargs):
        super(JsonBlackbox, self).__init__(*args, **kwargs)
        self._json_decode = kwargs.get('json_decode_function')

    def _get_json_decode(self):
        try:
            import ujson as json
        except ImportError:
            try:
                import json
            except ImportError:
                import simplejson as json

        return json.loads

    def json_decode(self, data):
        if not self._json_decode:
            self._json_decode = self._get_json_decode()
        if six.PY3:
            data = data.decode('utf-8')
        return self._json_decode(data)

    @retry(retry_on_exception=can_retry, stop_max_attempt_number=BLACKBOX_RETRY_ERRORS_ATTEMPTS)
    def _blackbox_json_call(self, method, **kwargs):
        """Выполняет запрос к JSON-API."""

        response = self._blackbox_call(method, format='json', **kwargs)
        decoded_response = self.json_decode(response)

        exception = decoded_response.get('exception')
        if exception:
            error = decoded_response.get('error', 'No error message supplied')
            raise BlackboxResponseError('%s: %s' % (exception, error))
        return decoded_response

    def login(self, login, password, userip, **kwargs):
        """Проверяет пару значений логин/пароль.

        Кроме того в случае успешной проверки данный метод может возвращать
        сведения об аккаунте пользователя из паспортной базы данных.

        Подробнее:
            http://doc.yandex-team.ru/blackbox/reference/MethodLogin.xml

        """
        return self._blackbox_json_call('login', login=login,
                                        password=password, userip=userip,
                                        **kwargs)

    def oauth(self, **kwargs):
        """Проверяет auth-токен, выданный сервисом oauth.yandex.ru.

        Если токен валиден, позволяет получить дополнительную информацию об
        аккаунте пользователя из паспортной базы данных.

        В качестве аргумента принимает:
            - oauth_token со значением токена,
            - либо headers c хедером Authorization вида:
                "Authorization: OAuth vF9dft4dfsfsdf".

        Подробнее:
            http://doc.yandex-team.ru/blackbox/reference/method-oauth.xml

        """
        return self._blackbox_json_call('oauth', **kwargs)

    def sessionid(self, userip, sessionid, host, **kwargs):
        """Проверяет валидность кук Session_id и sessionid2.

        В случае успешной аутентификации данный метод дополнительно может возвращать
        сведения об аккаунте пользователя из паспортной базы данных.

        Подробнее:
            http://doc.yandex-team.ru/blackbox/reference/MethodSessionID.xml

        """
        return self._blackbox_json_call('sessionid', userip=userip,
                                        sessionid=sessionid, host=host,
                                        **kwargs)

    def userinfo(self, **kwargs):
        """Позволяет получать сведения о пользователе без проверки авторизации.

        Возможны следующие варианты идентификации пользователя при вызове
        данного метода:
            1. по uid;
            2. по login (пользователь идентифицируется логином Паспорта —
               accounts.login );
            3. по паре значений sid/login (пользователь идентифицируется
               логином на указанном сервисе — subscription.login );
            4. по паре значений suid/sid (данный вариант идентификации доступен
               только для Почты и Народа).

        Пример:
            json_blackbox.userinfo(suid=<suid>, sid=<sid>)

        Также можно попросить пачку пользователей по uid:
            json_blackbox.userinfo(uid=[<uid_1>, <uid_2>, ...])

        Подробнее:
            http://doc.yandex-team.ru/blackbox/reference/MethodUserInfo.xml

        """
        if 'uid' in kwargs:
            uid = kwargs['uid']
            if not isinstance(uid, (int, six.string_types)):
                kwargs['uid'] = ','.join(str(u) for u in uid)

        return self._blackbox_json_call('userinfo', **kwargs)


# Слой совместимости

Blackbox = XmlBlackbox

HTTP_TIMEOUT = Blackbox.TIMEOUT

BLACKBOX_URL_PRODUCTION = Blackbox.URLS['other']['production']

BLACKBOX_URL_DEVELOPMENT = Blackbox.URLS['other']['development']

BLACKBOX_URL = Blackbox.URL


def caller(name):
    @wraps(getattr(Blackbox, name))
    def _decorator(*args, **kwargs):
        if BLACKBOX_URL != Blackbox.URL or HTTP_TIMEOUT != Blackbox.TIMEOUT:
            warnings.warn('You should not monkey-patch blackbox module! '
                          'Use XmlBlackbox or JsonBlackbox classes instead.',
                          DeprecationWarning)
        # Инстанцируем объект Blackbox с целью эмуляции старого поведения
        return getattr(Blackbox(BLACKBOX_URL, HTTP_TIMEOUT), name)(*args, **kwargs)

    return _decorator


sessionid = caller('sessionid')
login = caller('login')
userinfo = caller('userinfo')
user_ticket = caller('user_ticket')
uid = caller('uid')
subscription = caller('subscription')
is_email_valid = caller('is_email_valid')
list_emails = caller('list_emails')
sex = caller('sex')
is_lite = caller('is_lite')
oauth = caller('oauth')
country = caller('country')
_check_login_rule = caller('_check_login_rule')

if __name__ == '__main__':
    import sys

    method = sys.argv[1]
    if method == 'sessionid':
        session_id = sys.argv[2]
        result = sessionid(sessionid=session_id,
                           userip='1.2.3.4',
                           host='heroism.yandex.ru',
                           dbfields=[FIELD_LOGIN]
                           )

        print(result.__dict__)
    elif method == 'login':
        loginname = sys.argv[2]
        password = sys.argv[3]
        result = login(loginname,
                       password,
                       userip='87.250.242.209',
                       dbfields=[FIELD_LOGIN]
                       )

        print(result.__dict__)
