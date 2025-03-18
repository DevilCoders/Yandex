# coding=utf-8

import posixpath
from collections import OrderedDict
from urlparse import urlparse
from urllib import urlencode

import requests
from flask import request
from voluptuous import REMOVE_EXTRA, Schema

from antiadblock.configs_api.lib.const import SECRET_DECRYPT_CRYPROX_TOKEN, DECRYPT_TOKEN_HEADER_NAME, PARTNER_TOKEN_HEADER_NAME


def check_args(schema, extra=REMOVE_EXTRA):
    """
    Checks flask's current request's args against given `schema`.

    Expects each argument to be a single-argument, for convenience.

    :param dict schema: a voluptuous-compatible dict with schema
    :param extra: like :py:func:`Schema`'s `extra` parameter. Default behaviour is to drop extra params.
    :return: schema output result
    """
    return Schema(schema, extra=extra)(dict(request.args.items(multi=False)))


def getmembers(object):
    """Return all members of an object as (name, value) """
    for key in dir(object):
        try:
            yield key, getattr(object, key)
        except AttributeError:
            continue


class ListableEnum(object):
    """
    A fancy mixin to add `X.all` to your enum-like classes

    >>> class MyEnum(ListableEnum):
    ...     FOO = 'foo'
    ...     BAR = 'bar'
    >>> 'foo' in MyEnum.all()
    True
    >>> len(MyEnum.all())
    2
    >>> 'BAR' in MyEnum.key_values()
    True
    >>> MyEnum.key_values()['FOO']
    'foo'
    """
    @classmethod
    def all(cls):
        return [v for (n, v) in getmembers(cls) if n.upper() == n]

    @classmethod
    def key_values(cls):
        # expecting value is a number, we want exact order. getmembers(cls) returns all values sorted by key =(
        return OrderedDict(sorted([(k, v) for (k, v) in getmembers(cls) if k.upper() == k], key=lambda el: el[1]))


class CryproxClient(object):
    """
    Simple client for interacting with Cryprox
    """

    def __init__(self, host='cryprox.yandex.net', http_scheme='https', port=None):
        self.host = ':'.join([host, port]) if port else host
        self.http_scheme = http_scheme

    def get(self, url, **kw):
        """
        Python requests alias for sending requests to cryprox like partners proxy_pass
        :param url: request url
        :param kw: python requests params - headers, cookies, timeout, etc
        :return: python requests response object
        """
        _url = urlparse(url)
        return requests.get(_url._replace(netloc=self.host, scheme=self.http_scheme).geturl(),
                            timeout=5,
                            allow_redirects=False,
                            **kw)


class URLBuilder(object):
    """
    Easy-to-use URL builder

    Doctests:

    >>> URLBuilder('http://ya.ru/').foo[1].bar
    URLBuilder(base='http://ya.ru/foo/1/bar', params={})

    >>> str(URLBuilder('http://ya.ru/').foo[1].path(bar='ololo'))
    'http://ya.ru/foo/1/path?bar=ololo'

    >>> import urlparse as up
    >>> url = str(URLBuilder('http://ya.ru/').foo[1].path(foo='ololo',
    ...                                                   bar='pewpew'))
    >>> up.parse_qs(up.urlsplit(url).query)['foo']
    ['ololo']

    """

    def __init__(self, base, params={}):
        self.base = base
        self.params = params

    def __call__(self, **params):
        return URLBuilder(self.base, dict(self.params, **params))

    def __str__(self):
        result = self.base

        if self.params:
            result += '?'
            result += urlencode(self.params)

        return result

    def __getattr__(self, attr):
        return URLBuilder(posixpath.join(self.base, attr), self.params)

    def __getitem__(self, item):
        return self.__getattr__(str(item))

    def __repr__(self):
        return 'URLBuilder(base=%r, params=%r)' % (self.base, self.params)


def cryprox_decrypt_request(service_id, crypted_url, cryprox_client, current_app):
    from antiadblock.configs_api.lib.db_utils import get_active_config

    config = get_active_config(label_id=service_id)
    config_dict = config.as_dict()
    service_token = config_dict['data']['PARTNER_TOKENS'][0]

    if not crypted_url.startswith('http') and not crypted_url.startswith('/'):
        crypted_url = '//' + crypted_url
    parsed_url = urlparse(crypted_url)
    # По мотивам https://st.yandex-team.ru/ANTIADB-910:
    # Если ссылка уже с доменом, то мы возьмем домен из нее,
    # если же нет (относительная, напр. `/test/img.jpg`) - из поля domain в конфиге сервиса
    service_domain = parsed_url.netloc if parsed_url.netloc else config.service.domain
    current_app.logger.info(
        'Try to decrypt url {url} on domain {domain}'.format(url=crypted_url, domain=service_domain))

    return cryprox_client.get(crypted_url,
                              headers={
                                  DECRYPT_TOKEN_HEADER_NAME: SECRET_DECRYPT_CRYPROX_TOKEN,
                                  PARTNER_TOKEN_HEADER_NAME: service_token,
                                  'host': service_domain})
