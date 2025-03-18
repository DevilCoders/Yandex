# -*- coding: utf-8 -*-
from __future__ import absolute_import, division, print_function, unicode_literals

import json
import socket
from collections import namedtuple
from logging import getLogger

import requests
import six
from cachetools import TTLCache
from flask import request, redirect, abort
from flask_principal import Identity, Principal, AnonymousIdentity
from six.moves import urllib

from tvm import Tvm


BLACKBOX_URL = 'https://blackbox-ipv6.yandex-team.ru/blackbox'
BLACKBOX_AUTH_URL = 'https://passport.yandex-team.ru/auth?retpath={0}'
BLACKBOX_TVM_ID = 223

BLACKBOX_URL_TEST = 'https://pass-test.yandex.ru/blackbox'
BLACKBOX_AUTH_URL_TEST = 'https://passport-test.yandex.ru/auth?retpath={0}'

OAUTH_URL = 'https://oauth.yandex-team.ru/authorize?response_type=token&client_id={}'
OAUTH_URL_TEST = 'https://oauth-test.yandex.ru/authorize?response_type=token&client_id={}'

# handy field name aliases
FIELD_LOGIN = 'accounts.login.uid'
FIELD_FIO = 'account_info.fio.uid'
FIELD_EMAIL = 'account_info.email.uid'

login_exempt_views = set()


def login_exempt(view):
    """A decorator that can exclude a view from authentication and authorization processes."""
    login_exempt_views.add('{}.{}'.format(view.__module__, view.__name__))
    return view


class NeedRedirectError(Exception):
    def __init__(self, response):
        self.response = response


class AllCanIdentity(Identity):
    """
    Fake identity object used if authentication is disabled.
    """
    def can(self, _):
        return True


class Authenticator(object):
    """
    The one that hooks into request processor and manages permissions
    by querying:
        * blackbox (for authentication)
    """

    CACHE_MAX_SIZE = 500  # Should be more than enough (we don't have many users).
    # This should also work - don't think that using invalid cookie for a short time is a crime.
    # But this will allow us not to issue blackbox requests on every incoming request.
    # This will help in case blackbox and/or staff API blackouts. There have been incidents already.
    CACHE_TTL = 60 * 60

    @classmethod
    def _init_ttl_cache(cls):
        return TTLCache(maxsize=cls.CACHE_MAX_SIZE,
                        ttl=cls.CACHE_TTL)

    def __init__(self, app, passport=None, enable=True, oauth_scopes=None):
        self.app = app
        self._enable = enable
        if passport is None:
            passport = PassportClient()
        self._passport = passport
        self._session_id_passport_cache = self._init_ttl_cache()
        self._oauth_token_passport_cache = self._init_ttl_cache()
        self._oauth_scopes = oauth_scopes

        # setup flask application
        principal = Principal(app, skip_static=True, use_sessions=False)
        principal.identity_loader(lambda: self.load_identity())

        app.errorhandler(NeedRedirectError)(self.handle_need_redirect_error)

        self.app.config.setdefault('PASSPORT_ROOT_USERS', [])

    def handle_need_redirect_error(self, e):
        return e.response

    def load_identity(self):
        if not self._enable:
            return AllCanIdentity(id='root')

        if not request.endpoint:
            return AnonymousIdentity()

        view = self.app.view_functions.get(request.endpoint)
        dest = '{}.{}'.format(view.__module__, view.__name__)

        login = None
        user_ip = request.access_route[0] if request.access_route else "127.0.0.1"

        authorization_header = request.headers.get('Authorization')
        if authorization_header:
            login = self._oauth_token_passport_cache.get(authorization_header)
            if login is not None:
                return self._make_identity(login)
            result = self._passport.check_oauth_token(
                user_ip=user_ip,
                authorization_header=authorization_header,
                scopes=self._oauth_scopes
            )
            login = result.login

            if result.error:
                abort(401, description=result.error)
            else:
                self._oauth_token_passport_cache[authorization_header] = login
                return self._make_identity(login)

        # Let us check if Session_id cookie value is cached.
        session_id = request.cookies.get('Session_id')
        if session_id:
            login = self._session_id_passport_cache.get(session_id)
            if login is not None:
                return self._make_identity(login)
        # Okay, let's call passport.
        result = self._passport.check_passport_cookie(
            cookies=request.cookies,
            host=request.host,
            user_ip=user_ip,
            request_url=request.url
        )
        if result.redirect_url:  # Cookie is invalid
            if dest in login_exempt_views:
                return AnonymousIdentity()
            # This method must return identity
            # so we break call stack by raising exception
            # and catching it using flask error handlers mechanism
            # and responding with redirect
            response = redirect(result.redirect_url)
            raise NeedRedirectError(response)
        elif result.login:
            login = result.login
            # Put login into cache
            self._session_id_passport_cache[session_id] = login
        return self._make_identity(login)

    def _make_identity(self, login):
        if login in self.app.config['PASSPORT_ROOT_USERS']:
            return AllCanIdentity(login)
        return Identity(login)


PassportCheckResult = namedtuple('PassportCheckResult', ['login', 'redirect_url'])
OAuthCheckResult = namedtuple('OAuthCheckResult', ['login', 'client_id', 'error'])


class PassportClient(object):
    _REQ_TIMEOUT = 10

    @classmethod
    def from_config(cls, d):
        return cls(blackbox_url=d.get('blackbox_url'),
                   blackbox_auth_url=d.get('blackbox_auth_url'),
                   blackbox_tvm_id=d.get('blackbox_tvm_id'),
                   req_timeout=d.get('req_timeout'),
                   tvm_id=d.get('tvm_id'),
                   tvm_secret=d.get('tvm_secret'))

    def __init__(self, blackbox_url=BLACKBOX_URL, blackbox_auth_url=BLACKBOX_AUTH_URL, blackbox_tvm_id=BLACKBOX_TVM_ID,
                 tvm_id=None, tvm_secret=None, req_timeout=None):
        self._blackbox_url = blackbox_url
        self._blackbox_auth_url = blackbox_auth_url
        self._req_timeout = req_timeout or self._REQ_TIMEOUT
        self._tvm = None
        if tvm_id and tvm_secret:
            self._tvm = Tvm(tvm_id, tvm_secret, blackbox_tvm_id=blackbox_tvm_id)
        self._session = requests.Session()
        self._log = getLogger(__name__)

    def check_passport_cookie(self, cookies, host, user_ip, request_url):
        """
        Check passport cookie.
        :param cookies: dict-like object containing cookies,
                        e.g. :attr:`flask.request.cookies`)
        :param host: requested host, e.g. :attr:`flask.request.host`
        :param user_ip: user ip, e.g. the first element of :attr:`flask.request.access_route`
        :param request_url: URL to redirect a user to after successful authentication
        :rtype: PassportCheckResult
        """
        session_id = cookies.get('Session_id')
        if not session_id:
            return PassportCheckResult(login=None, redirect_url=self._blackbox_auth_url.format(request_url))
        if ':' in host:
            host = host.split(':')[0]
        valid, need_redirect, dbfields = validate_session_id(session_id,
                                                             user_ip, host,
                                                             [FIELD_LOGIN],
                                                             timeout=self._req_timeout,
                                                             url=self._blackbox_url,
                                                             session=self._session,
                                                             tvm=self._tvm)
        if not valid or need_redirect:
            return PassportCheckResult(login=None, redirect_url=self._blackbox_auth_url.format(request_url))
        # put login to lowercase
        # otherwise users like 'nARN' can get in trouble
        login = dbfields[FIELD_LOGIN].lower()
        self._log.info("authenticated via blackbox: login='{0}' ip='{1}'".format(login, user_ip))
        return PassportCheckResult(login=login, redirect_url=None)

    def check_oauth_token(self, user_ip, oauth_token=None, authorization_header=None, scopes=None):
        """
        Check OAuth token.
        :param oauth_token: OAuth token or entire content of Authorization header
        :param user_ip: user ip, e.g. the first element of :attr:`flask.request.access_route`
        :param scopes: list of required token scopes
        :rtype: OAuthCheckResult
        """
        assert (oauth_token or authorization_header) and not (oauth_token and authorization_header)
        valid, fields, client_id, error = validate_oauth_token(
            oauth_token=oauth_token,
            authorization_header=authorization_header,
            userip=user_ip,
            fields=[FIELD_LOGIN],
            timeout=self._req_timeout,
            url=self._blackbox_url,
            session=self._session,
            tvm=self._tvm,
            scopes=scopes)
        if valid:
            return OAuthCheckResult(login=fields[FIELD_LOGIN], client_id=client_id, error=None)
        else:
            return OAuthCheckResult(login=None, client_id=client_id, error=error)


class BlackboxError(Exception):
    pass


# Django copy-paste ->

def force_bytes(s):
    # Handle the common case first for performance reasons.
    if isinstance(s, bytes):
        return s

    if isinstance(s, six.string_types):
        return s.encode('utf-8')

    if six.PY3:
        return six.text_type(s).encode('utf-8')

    try:
        return bytes(s)
    except UnicodeEncodeError:
        return six.text_type(s).encode('utf-8')


def urlencode(params):
    """
    A version of Python's urllib.urlencode() function that can operate on
    unicode strings. The parameters are first case to UTF-8 encoded strings and
    then encoded as per normal.
    """
    if hasattr(params, 'items'):
        params = params.items()
    return urllib.parse.urlencode(
        [(force_bytes(k),
          isinstance(v, (list, tuple)) and [force_bytes(i) for i in v] or force_bytes(v))
         for k, v in params])

# <- Django copy-paste


def join_url_params(url, params):
    # === sanitize ===
    dbfields = params.get('dbfields')
    if dbfields is not None:
        params['dbfields'] = ','.join(dbfields)
    ipv6_prefix = '::ffff:'
    userip = params.get('userip')
    if userip is not None and userip.startswith(ipv6_prefix):
        params['userip'] = userip[len(ipv6_prefix):]
    # === join ===
    params = urlencode(params)
    parts = list(urllib.parse.urlparse(url))
    path = parts[2]
    if not path.startswith('/'):
        path = '/{0}'.format(path)
    path = '{0}?{1}'.format(path, params)
    parts[2] = path
    return urllib.parse.urlunparse(parts)


def _http_get(url, params, timeout=None, headers=None, session=None):
    url = join_url_params(url, params)
    try:
        if session is None:
            session = requests
        resp = session.get(url, headers=headers, timeout=timeout)
    except socket.error as e:
        raise BlackboxError('socket error: {0}'.format(e.strerror))
    except requests.RequestException as e:
        raise BlackboxError('connection error: {0}'.format(str(e)))
    # according to blackbox documentation service always returns status 200
    # any other status code is treated as error
    if resp.status_code != 200:
        raise BlackboxError('bad http status: {0}'.format(resp.status_code))
    content_type = resp.headers.get('content-type', 'text/xml').split(';')[0]
    return resp.text, content_type


def _blackbox_json_call(url,  params, timeout=None, headers=None, session=None, tvm=None):
    if headers is None:
        headers = {}
    if tvm is not None:
        headers["X-Ya-Service-Ticket"] = tvm.get_service_ticket()
    params['format'] = 'json'
    data, content_type = _http_get(url, params, timeout, headers=headers, session=session)
    if content_type != 'application/json':
        raise BlackboxError("received content type '{0}', "
                            "was waiting for JSON".format(content_type))
    return json.loads(data)


def validate_session_id(sessionid, userip, host,
                        fields=None, timeout=None, url=BLACKBOX_URL, session=None, tvm=None):
    """
    Check provided sessionid for validity.
    Futher reading:
        http://doc.yandex-team.ru/blackbox/reference/MethodSessionID.xml

    Possible results:
        * socket.error in case of TCP failure
        * gevent.Timeout in case of timeout
        * BlackboxError in case of:
            * bad HTTP code
            * ACL/format error
            * internal error
        * success
    """
    params = {
        'method': 'sessionid',
        'sessionid': sessionid,
        'userip': userip,
        'host': host,
        'dbfields': fields
    }
    result = _blackbox_json_call(url, params, timeout, session=session, tvm=tvm)
    error = result['error']
    if error != "OK":
        raise BlackboxError(error)
    status = result['status']['value']
    age = result.get('age', 0)
    valid = (status == 'VALID' or status == 'NEED_RESET')
    redirect = (status == 'NEED_RESET' or (status == 'NOAUTH' and age < 2 * 60 * 60))
    fields = result['dbfields'] if valid else None
    return valid, redirect, fields


def validate_oauth_token(userip, oauth_token=None, authorization_header=None, fields=None,
                         timeout=None, url=BLACKBOX_URL, session=None, tvm=None, scopes=None):
    """
    Wrapper for http://doc.yandex-team.ru/blackbox/reference/method-oauth.xml
    """
    params = {
        'method': 'oauth',
        'userip': userip,
        'dbfields': fields
    }
    if scopes:
        params['scopes'] = ','.join(scopes)
    headers = {}

    if authorization_header:
        headers['Authorization'] = authorization_header
    elif oauth_token:
        params['oauth_token'] = oauth_token
    else:
        raise RuntimeError('either oauth_token or authorization_header must be specified')

    result = _blackbox_json_call(url, params, timeout, headers=headers, session=session, tvm=tvm)
    error = result['error']
    if error != "OK":
        raise BlackboxError(error)

    # http://doc.yandex-team.ru/blackbox/reference/method-oauth-response-json.xml
    status = result['status']['value']
    valid = status == 'VALID'
    fields = result['dbfields'] if valid else None
    client_id = result['oauth']['client_id'] if valid else None
    error = result['error'] if not valid else None
    return valid, fields, client_id, error


class OAuth(object):
    def __init__(self, client_id, oauth_url=OAUTH_URL):
        self.client_id = client_id
        self.redirect_url = oauth_url.format(client_id)

    def authorize(self):
        return redirect(self.redirect_url)
