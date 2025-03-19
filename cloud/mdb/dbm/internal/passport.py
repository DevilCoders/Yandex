"""
Yandex Blackbox-related helpers
"""
import threading
from typing import Optional
from flask import abort, current_app, g, redirect, request
import blackbox
from ticket_parser2.api.v1 import BlackboxClientId  # type: ignore

from dbaas_common import tracing
from .config import app_config

_bb_client: Optional[blackbox.JsonBlackbox] = None
_bb_init_lock = threading.Lock()


def _new_bb_client() -> blackbox.JsonBlackbox:
    config = app_config()
    url = config['BLACKBOX_URL']
    if config.get('USE_TVM'):
        blackbox_client_id = getattr(BlackboxClientId, app_config()['TVM_BLACKBOX_ENV'])
        return blackbox.JsonBlackbox(
            url=url,
            tvm2_client_id=config['TVM_CLIENT_ID'],
            tvm2_secret=config['TVM_SECRET'],
            blackbox_client=blackbox_client_id,
        )
    return blackbox.JsonBlackbox(url=url)


def _get_blackbox() -> blackbox.JsonBlackbox:
    """
    Get blackbox client
    """
    global _bb_client
    if _bb_client is None:
        with _bb_init_lock:
            if _bb_client is None:
                _bb_client = _new_bb_client()
    return _bb_client


_CHECK_SCOPE = 'pgaas:all'


@tracing.trace('Passport Check OAuth')
def _check_oauth(address):
    """
    Validate OAuth token from headers with blackbox
    """
    token = request.headers.get('Authorization')
    if not token:
        return False, None

    token = token.replace('OAuth ', '')
    bb = _get_blackbox()
    result = bb.oauth(oauth_token=token, userip=address)
    current_app.logger.debug(result)

    if not result.get('status', {}).get('value', '') == 'VALID':
        return False, None
    if not result.get('oauth', {}).get('scope', '') == _CHECK_SCOPE:
        return False, None
    allowed = False
    if result['login'] in app_config()['ALLOWED_LOGINS']:
        allowed = True
    return allowed, result['login']


@tracing.trace('Passport Check')
def _check(ret_path):
    """
    Check auth or redirect user to bb if auth check is failed
    """
    config = app_config()
    redirect_url = 'https://passport.yandex-team.ru/auth?retpath=http://' + config['BASE_HOST'] + ret_path

    if 'HTTP_X_FORWARDED_FOR' in request.environ:
        address = request.environ.get('HTTP_X_FORWARDED_FOR')
    else:
        address = request.environ.get('REMOTE_ADDR')
    sessionid = request.cookies.get('Session_id')
    if not sessionid:
        allowed, login = _check_oauth(address)
        current_app.logger.info('Allowed: %s, Login: %s', allowed, login)
        if allowed:
            g.login = login  # pylint: disable=assigning-non-slot
            return None
        return abort(403)
    bb = _get_blackbox()
    result = bb.sessionid(address, sessionid, config['BASE_HOST'])
    current_app.logger.debug(result)
    if result['status']['value'] != 'VALID':
        return redirect(redirect_url, code=307)
    if result['login'] in config['ALLOWED_LOGINS']:
        g.login = result['login']  # pylint: disable=assigning-non-slot
        return None
    return abort(403)


def check_auth(return_path='/'):
    """
    Passport auth check decorator
    """

    def wrapper(callback):
        """
        Wrapper function (returns internal wrapper)
        """

        def auth_check_wrapper(*args, **kwargs):
            """
            Run callback if auth check was ok
            """
            check_result = _check(return_path)
            if check_result:
                return check_result

            return callback(*args, **kwargs)

        auth_check_wrapper.__name__ = callback.__name__
        auth_check_wrapper.__doc__ = callback.__doc__
        auth_check_wrapper.__dict__.update(callback.__dict__)
        return auth_check_wrapper

    return wrapper
