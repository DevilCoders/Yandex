# -*- coding:utf-8 -*-

import six
import hashlib
import hmac
import warnings
from base64 import urlsafe_b64decode, urlsafe_b64encode
from datetime import datetime
from time import time
if six.PY2:
    from urllib2 import urlopen
    from urlparse import urlparse
    from urllib import quote
elif six.PY3:
    from urllib.request import urlopen
    from urllib.parse import urlparse, quote

import xml.etree.ElementTree as ET
from xml.parsers.expat import ExpatError

from django.conf import settings

import yenv


def get_yauth_type(request=None):
    default = settings.YAUTH_TYPE or (
        'intranet' if yenv.name == 'intranet' else 'desktop')

    if request is not None:
        return getattr(request, 'yauth_type', default)

    return default


def get_real_ip(request):
    for header in settings.YAUTH_REAL_IP_HEADERS:
        if header in request.META:
            return request.META[header].split(',')[0]
    return None


def get_passport_host(request, yauth_type=None):
    scheme, netloc, _path, _params, _query, _fragments = urlparse(get_passport_url('create', yauth_type=yauth_type, request=request))
    return '%s://%s' % (scheme, netloc)


def get_passport_url(url_type, yauth_type=None, request=None, retpath=None):
    if url_type == 'create' and settings.YAUTH_SESSIONID2_REQUIRED:
        # Может возникнуть ситуация, когда при валидной Session_id нет куки sessionid2,
        # тогда ее нужно создать в паспорте на специальной форме доввода пароля
        url_type = 'create_secure'

    if not yauth_type:
        yauth_type = get_yauth_type(request)

    passport_url = get_setting(['PASSPORT_URLS', 'YAUTH_PASSPORT_URLS'])[yauth_type][url_type]

    if request:
        # Для TLD .com кука нужна от .com-паспорта
        # Интранет у нас только .ru, поэтому для него ничего не исправляем
        # TODO: вынести вариацию паспорта от хоста в настройки.
        # иначе получается плохо, что мы меняем пользовательские настройки url паспорта
        host = get_current_host(request)

        if yauth_type not in ['intranet', 'intranet-testing']:
            for tail in settings.YAUTH_AVAILABLE_TLDS:
                if host.endswith(tail):
                    passport_url = passport_url.replace('.ru/', '%s/' % tail, 1)

                    break

    if retpath:
        if retpath is True:
            retpath = get_current_url(request)

        passport_url += quote(retpath)

    return passport_url


def get_current_host(request=None, drop_port=True):
    if settings.YAUTH_USE_SITES:
        from django.contrib.sites.models import Site

    if settings.YAUTH_USE_SITES and Site._meta.installed:
        site = Site.objects.get_current()
        host = site.domain
    elif request is not None:
        if settings.YAUTH_HOST_HEADER and settings.YAUTH_HOST_HEADER in request.META:
            host = request.META[settings.YAUTH_HOST_HEADER]
        else:
            host = request.get_host()
    else:
        raise ValueError('`get_current_host` requires either installed Site model'
                         ' (with YAUTH_USE_SITES=True in settings)'
                         ' or passing a request instance')

    if drop_port:
        host = host.split(':')[0]

    return host


def get_current_url(request, add_params=None, del_params=None):
    """
    @rtype: bytes
    """
    host = get_current_host(request, drop_port=False)
    schema = 'https' if request.is_secure() else 'http'
    url = ('%s://%s%s' % (schema, host, request.path)).encode('utf-8')
    params = request.GET
    if add_params or del_params:
        params = request.GET.copy()
        params.update(add_params or {})
        for p in del_params or []:
            del params[p]
    if params:
        url += b'?' + params.urlencode().encode('utf-8')
    return url


class SubscribeError(Exception):
    pass


def subscribe_user(uid, yauth_type=None):
    '''
    Прямая подписка юзера на сервис. Возвращает созданный SID или
    False в случае неудачи или отсутствии прав.
    '''
    if not yauth_type:
        yauth_type = get_yauth_type()

    url = get_passport_url('admsubscribe', yauth_type) % {
        'slug': get_setting(['PASSPORT_SERVICE_SLUG', 'YAUTH_PASSPORT_SERVICE_SLUG']),
        'uid': uid,
    }
    try:
        root = ET.parse(urlopen(url)).getroot()
    except (ExpatError, ET.ParseError):
        raise SubscribeError('Invalid XML. Check your admsubscribe grants', '')
    if root.attrib['job'] != 'accepted':
        return False
    else:
        return int(root.find('sid').text)


def debug_msg(request):
    if 'Session_id' not in request.COOKIES:
        msg = "No Session_id cookie. Possible reasons: 1. domain doesn't" \
              " correspond to passport domain; 2. browser cookies disabled"
    elif (hasattr(request.yauser, 'blackbox_result')
            and request.yauser.blackbox_result is None):
        msg = 'Blackbox unavailable'
    elif (hasattr(request.yauser, 'blackbox_result')
            and not request.yauser.blackbox_result.valid):
        r = request.yauser.blackbox_result
        msg = 'Invalid response from %s: status=%s, error=%r' % (
            getattr(r, 'url', 'Blackbox'), r.status, r.error)
    else:
        return ''
    return ' (' + msg + ')'


def get_setting(names):
    if not isinstance(names, (list, tuple)):
        names = (names, )

    for name in names:
        try:
            return getattr(settings, name)
        except AttributeError:
            continue

    raise AttributeError('No setting with specified names: %s' % ', '.join(names))


def parse_time(time_str):
    return datetime.strptime(time_str, '%Y-%m-%d %H:%M:%S')


# Depricated functions


def current_url(*args, **kwargs):
    warnings.warn('`current_url` is deprecated. Use `get_current_url` instead', DeprecationWarning, stacklevel=2)

    return get_current_url(*args, **kwargs)


def current_host(*args, **kwargs):
    warnings.warn('`current_host` is deprecated. Use `get_current_host` instead', DeprecationWarning, stacklevel=2)

    return get_current_host(*args, **kwargs)


def generate_tvm_ts_sign(tvm_client_secret):
    # https://wiki.yandex-team.ru/passport/tvm/api/#primerpoluchenijapodpisi
    padded_secret = tvm_client_secret + '=' * (-len(tvm_client_secret) % 4)
    decoded_secret = urlsafe_b64decode(str(padded_secret))
    ts = int(time())
    ts_sign = urlsafe_b64encode(
        hmac.new(
            key=decoded_secret,
            msg=str(ts).encode('UTF-8'),
            digestmod=hashlib.sha256,
        ).digest(),
    ).decode('UTF-8')

    return ts, ts_sign
