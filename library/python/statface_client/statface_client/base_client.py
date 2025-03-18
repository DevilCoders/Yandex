# coding: utf8

from __future__ import division, absolute_import, print_function, unicode_literals

import logging
import os
import sys
import contextlib
import uuid

import yaml
import requests
import six
from requests.packages.urllib3.util import Retry  # pylint: disable=ungrouped-imports

from statface_client import version
from .tools import (
    NestedDict, find_caller, is_arcadia, get_cacert_path,
    get_process_context,
)
from .errors import StatfaceClientAuthError
from .errors import StatfaceClientAuthConfigError
from .errors import StatfaceHttpResponseRetriableError, StatfaceHttpResponseFatalError
from .errors import StatfaceClientRetriableError
from .constants import (
    STATFACE_BETA,
    ENDLESS_WAIT_FOR_FINISH,
    RETRIABLE_STATUSES,
    UNRETRIABLE_STATUSES,
)
from .errors import StatfaceClientValueError

logger = logging.getLogger('statface_client')

DEFAULT_AUTH_PATH = '.statbox/statface_auth.yaml'

DEFAULT_STATFACE_CONFIG = {
    'username': None,
    'password': None,
    'oauth_token': None,
    'host': STATFACE_BETA,
    # try to get auth info from auth config
    'use_auth_config': True,
    # $HOME/.statbox/statface_auth.yaml by default
    'auth_config_path': None,
    'reports': {
        'upload_data': {
            # just for backward compatibility: STATINFRA-8366
            'host': None,
            'use_upload_id': True,
            # possible values:
            # JUST_ONE_CHECK, DO_NOTHING, ENDLESS_WAIT_FOR_FINISH
            'check_action_in_case_error': ENDLESS_WAIT_FOR_FINISH,
        }
    },

    'dictionaries': {}
}


#
# _SRCFILE is used when walking the stack to check when we've got the first
# caller stack frame to construct requests User-Agent
# Almost copypaste from `logging/__init__.py`
#
if hasattr(sys, 'frozen'):  # support for py2exe
    _SRCFILE = "base_client%s__init__%s" % (os.sep, __file__[-4:])
elif __file__[-4:].lower() in ['.pyc', '.pyo']:
    _SRCFILE = __file__[:-4] + '.py'
else:
    _SRCFILE = __file__
_SRCFILE = os.path.dirname(os.path.normcase(_SRCFILE))


class ClientConfig(NestedDict):

    def __init__(self, client_config):
        super(ClientConfig, self).__init__(DEFAULT_STATFACE_CONFIG)
        try:
            self.update(NestedDict(client_config))
        except (KeyError, AttributeError, ValueError) as e:
            text = \
                'client_config mismatches DEFAULT_STATFACE_CONFIG ' + \
                'structure: {} {}'
            raise StatfaceClientValueError(text.format(type(e), str(e)))

    @staticmethod
    def get_auth_from_path(path=None):
        if not path:
            auth_path = os.path.join(
                os.path.expanduser('~'),
                DEFAULT_AUTH_PATH
            )
        else:
            auth_path = path
        try:
            with open(auth_path) as fobj:
                auth = yaml.safe_load(fobj)
            return dict(auth)
        except Exception as exc:
            raise StatfaceClientAuthConfigError(exc)

    def load_auth_config_if_needed(self):
        if (not (self['oauth_token'] or (self['username'] and self['password'])) and
                self['use_auth_config']):
            auth = self.get_auth_from_path(self['auth_config_path'])
            self['oauth_token'] = self['oauth_token'] or auth.get('oauth_token')
            self['username'] = self['username'] or auth.get('username')
            self['password'] = self['password'] or auth.get('password')

    def check_valid(self):
        if self.get('host') is None:
            raise StatfaceClientValueError('specify Statface host')
        if self.get('oauth_token') is None:
            for field in ('username', 'password'):
                if self.get(field) is None:
                    raise StatfaceClientValueError('specify Statface {}'.format(field))


def _hide_password(password, _symbols_left=2):
    if password is None:
        return None
    return password[:_symbols_left] + '***'


def _hide_oauth(oauth):
    if oauth is None:
        return None
    oauth = str(oauth)
    if oauth.startswith('OAuth '):
        prefix = oauth[:6]
        oauth = oauth[6:]
    else:
        prefix = ''
    # AQAD-ab***
    return prefix + _hide_password(oauth, 7)


def _hide_password_in_headers(headers):
    headers = dict(headers)
    if headers.get('StatRobotPassword') is not None:
        password = headers['StatRobotPassword']
        headers.update({'StatRobotPassword': _hide_password(password)})
    if headers.get('Authorization') is not None:
        oauth = headers['Authorization']
        headers.update({'Authorization': _hide_oauth(oauth)})
    return headers


@contextlib.contextmanager
def _wrap_request_error(request):
    try:
        yield
    except StatfaceHttpResponseFatalError:
        raise
    except StatfaceHttpResponseRetriableError:
        raise
    except requests.exceptions.RequestException as exc:
        _, _, traceback_ = sys.exc_info()
        request.headers = _hide_password_in_headers(request.headers or {})
        if exc.response is not None and exc.response.status_code in UNRETRIABLE_STATUSES:
            error = StatfaceHttpResponseFatalError(exc.response, request, original_error=exc)
        else:
            error = StatfaceHttpResponseRetriableError(exc.response, request, original_error=exc)
        logger.warning("Statface client wrapped error: %s", exc)
        six.reraise(type(error), error, traceback_)


class BaseStatfaceClient(object):

    DEFAULT_PROTOCOL = 'https://'

    _global__version_logged = False
    default_log_level = logging.INFO

    def __init__(   # pylint: disable=too-many-arguments
        self,
        username=None,
        password=None,
        host=None,
        client_config=None,
        oauth_token=None,
        _no_excess_calls=False,
        retries_num=5,
        retries_backoff_factor=1,
        retries_status_forcelist=(500, 502, 503, 504, 521),
        retries_method_whitelist=frozenset(['HEAD', 'TRACE', 'GET', 'PUT',
                                            'OPTIONS', 'DELETE', 'POST']),
    ):

        if not BaseStatfaceClient._global__version_logged:
            logger.log(self.default_log_level, "Initializing python-statface-client==%s", version.__version__)
            BaseStatfaceClient._global__version_logged = True

        client_config = client_config or {}
        client_config = dict(client_config)
        if oauth_token is not None:
            client_config['oauth_token'] = oauth_token
        if username is not None:
            client_config['username'] = username
        if password is not None:
            client_config['password'] = password
        if host is not None:
            client_config['host'] = host
        self._no_excess_calls = _no_excess_calls

        self._config = ClientConfig(client_config)
        self._config.load_auth_config_if_needed()
        self._config.check_valid()

        self._session = requests.session()

        if not is_arcadia():
            cacert_path = get_cacert_path()

            logger.debug('Setting requests _session.verify to `%s`', cacert_path)
            self._session.verify = cacert_path

        for prefix in ['http://', 'https://']:
            self._session.mount(
                prefix,
                requests.adapters.HTTPAdapter(
                    max_retries=Retry(
                        total=retries_num,
                        backoff_factor=retries_backoff_factor,
                        status_forcelist=retries_status_forcelist,
                        method_whitelist=retries_method_whitelist,
                        # https://stackoverflow.com/a/43496895
                        raise_on_status=False,
                    ),
                ),
            )

    @property
    def config(self):
        return self._config

    @property
    def oauth_token(self):
        return self.config['oauth_token']

    @property
    def username(self):
        return self.config['username']

    @property
    def password(self):
        return self.config['password']

    @property
    def host(self):
        return self.config['host']

    @property
    def _auth_headers(self):
        if self.oauth_token is not None:
            return {'Authorization': 'OAuth {}'.format(self.oauth_token)}
        return {
            'StatRobotUser': self.username,
            'StatRobotPassword': self.password,
        }

    @staticmethod
    def _context_headers(headers=None):
        """
        Make a verbose 'User-Agent' and, if available, 'X-Context-Url'.
        """
        headers = headers or {}
        result = {}

        # Making verbose User-Agent
        cfile, cline, cfunc = find_caller(extra_depth=1, skip_packages=(_SRCFILE,))
        prev_ua = headers.get("User-Agent") or requests.utils.default_user_agent()

        process_context = get_process_context() or {}
        name = process_context.get('name') or ''

        if name:
            name = ' @ {}'.format(name)

        result["User-Agent"] = (
            '{ua}, python-statface-client {version}{name},'
            ' {cfile}:{cline}: {cfunc}').format(
                ua=prev_ua,
                version=version.__version__,
                name=name,
                cfile=cfile,
                cline=cline,
                cfunc=cfunc,
            )

        url = process_context.get('url')
        if url:
            result['X-Context-Url'] = url

        reqid = process_context.get('x_request_id')
        reqid = '{}{}'.format(
            '{}__'.format(reqid) if reqid else '',
            str(uuid.uuid4()))
        result['X-Request-Id'] = reqid

        return result

    def _prepare_request(self, method, uri, host=None, **kwargs):
        if host is None:
            host = self.host
        if not (host.startswith('https://') or host.startswith('http://')):
            host = self.DEFAULT_PROTOCOL + host

        url = '{host}/{uri}'.format(
            host=host.rstrip('/'),
            uri=uri.lstrip('/'),
        )

        headers = kwargs.setdefault('headers', {})

        headers.update(self._auth_headers)
        headers.update(self._context_headers(headers))

        info = dict(method=method, url=url, headers=headers)
        kwargs.update(info)

        logger.log(
            self.default_log_level,
            "%s %s/%s  headers=%s",
            method.upper(), host, uri,
            _hide_password_in_headers(headers),
        )
        return requests.Request(**kwargs)

    def _request(   # pylint: disable=too-many-locals
            self,
            method,
            uri,
            host=None,
            raise_for_status=True,
            **kwargs):

        # There's generally no point in waiting longer for any sinle request.
        # (note that this timeout doesn't include retries...)
        kwargs.setdefault('timeout', 1300)

        prepared_request_keys = set((
            'headers', 'files', 'data', 'params',
            'auth', 'cookies', 'hooks', 'json'
        ))
        send_keys = set(('stream', 'timeout', 'verify', 'cert', 'proxies'))
        prepared_request_kwargs = {}
        send_kwargs = {}
        for key, value in six.iteritems(kwargs):
            if key in prepared_request_keys:
                prepared_request_kwargs[key] = value
            elif key in send_keys:
                send_kwargs[key] = value
            else:
                raise StatfaceClientValueError(
                    'unexpected key {} in _request kwargs'.format(key)
                )

        raw_request = self._prepare_request(
            method, uri, host, **prepared_request_kwargs
        )

        with _wrap_request_error(raw_request):
            request = self._session.prepare_request(raw_request)
            response = self._session.send(request, **send_kwargs)
            try:
                elapsed = '%.3fs' % (response.elapsed.total_seconds(),)
            except Exception as exc:    # pylint: disable=broad-except
                elapsed = '???(%s)' % (repr(exc)[:32],)
            logger.log(
                self.default_log_level,
                "Response: %s %s %s   %db in %s resp_headers=(%r)",
                response.status_code,
                response.request.method,
                response.url,
                len(response.content or ''),
                elapsed,
                response.headers)
            if raise_for_status:
                if response.status_code in UNRETRIABLE_STATUSES:
                    raise StatfaceHttpResponseFatalError(response, raw_request)
                if response.status_code in RETRIABLE_STATUSES:
                    raise StatfaceHttpResponseRetriableError(response, raw_request)

        return response

    def check_auth(self):
        response = self._request(
            'get',
            '/_api/currenttime/',
            raise_for_status=False
        )
        if response.status_code in (401, 403):
            raise StatfaceClientAuthError(
                'invalid auth credentials. username: {}, password: {}, oauth_token: {}'.format(
                    self.username,
                    _hide_password(self.password),
                    _hide_oauth(self.oauth_token)
                )
            )
        if response.status_code in RETRIABLE_STATUSES:
            raise StatfaceClientRetriableError(
                'internal error while checking auth credentials with message: %s' % response.text
            )
