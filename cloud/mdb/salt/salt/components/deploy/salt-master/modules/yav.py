# -*- coding: utf-8 -*-
'''
This module helps to use Yandex Vault with salt.

:depends: yandex-passport-vault-client (https://vault-api.passport.yandex.net/docs/#yav)
It can be installed with
.. code-block:: bash

    $ pip install yandex-passport-vault-client -i https://pypi.yandex-team.ru/simple

This is often useful if you wish to store your pillars in source control or
share your pillar data with others but do not store secrets in them.

:configuration: The following configuration should be provided
    define (pillar or config files) Check that private keyfile is owned by root and has correct chown (400)`:

    .. code-block:: python

        # cat /etc/salt/master.d/yav.conf
        yav.config:
            rsa-private-key: /root/.ssh/id_rsa
            rsa-login: your-robot-login

Pillar files can include yav secret uids and they will be replaced by secret values while compilation:

.. code-block:: jinja

    pillarexample:
        user: root
        some-token-list: {{salt.yav.get('ver-01cr01gy78qv1a33rtp538ryyc')|json}}
        some-token: {{salt.yav.get('ver-01cr01gy78qv1a33rtp538ryyc[TOKEN]')|json}}
        server-certificate: {{salt.yav.get('sec-01cqywscqcd9jed163wv1nbzx8[server.crt]')|json}}
        server-certificate-key: {{salt.yav.get('sec-01cqywscqcd9jed163wv1nbzx8[server.key]')|json}}

If specific key not set then dict will be returned.
In pillars rended with jinja be sure to include `|json` so line breaks are encoded:

.. code-block:: jinja

    cert: "{{salt.yav.get('S2uogToXkgENz9...085KYt')|json}}"

In states rendered with jinja it is also good pratice to include `|json`:

.. code-block:: jinja

    {{sls}} private key:
        file.managed:
            - name: /etc/ssl/private/cert.key
            - mode: 700
            - contents: {{pillar['pillarexample']['server-certificate-key']|json}}

'''
# Import Python libs
from __future__ import absolute_import, print_function, unicode_literals

import base64
import fnmatch
import json
import logging
import re
import time
from collections import OrderedDict
from functools import partial

import redis
# Import Salt libs
import salt.syspaths
import salt.utils.files
import salt.utils.platform
import salt.utils.stringutils
import salt.utils.versions
import salt.utils.win_dacl
import salt.utils.win_functions

REQ_ERROR = None
try:
    from vault_client import VaultClient
    from vault_client.auth import (RSAPrivateKeyAuth, RSASSHAgentHash)
    from vault_client.errors import ClientError
    from vault_client.instances import Production, Custom
except (ImportError, OSError):
    REQ_ERROR = 'yav_client import error, perhaps missing python yandex-passport-vault-client package.'

try:
    import six
except ImportError:
    from salt.ext import six


__virtualname__ = 'yav'

log = logging.getLogger(__name__)

YAV_RE = re.compile(r'^(?P<uuid>(?:sec|ver)-[0-9a-z]{26,})(?:\[(?P<keys>.+?)\])?$', re.I)


def __virtual__():
    return check_requirements()


def check_requirements():
    '''
    Check required libraries are available
    '''
    return (REQ_ERROR is None, REQ_ERROR)


def _get_config(**kwargs):
    '''
    Return configuration
    '''
    config = {
        'rsa-private-key': None,
        'rsa-login': None,
        'redis-pass': None,
    }

    config_key = '{0}.config'.format(__virtualname__)
    try:
        config.update(__salt__['config.get'](config_key, {}))
    except (NameError, KeyError):
        # likly using salt-run so fallback to __opts__
        config.update(__opts__.get(config_key, {}))
    # pylint: disable=C0201
    for k in set(config.keys()) & set(kwargs.keys()):
        config[k] = kwargs[k]

    return config


def _get_yav_client(config):
    if 'yav_client' in __context__:
        return __context__['yav_client']

    vault_client_factory = Production
    if config.get('host'):
        vault_client_factory = partial(Custom, host=config.get('host'), check_status=True)

    vault_client = None
    # create Vault client
    if config.get('oauth-token'):
        vault_client = vault_client_factory(authorization='Oauth {}'.format(config.get('oauth-token')))
    elif config.get('rsa-login') and config.get('rsa-private-key'):
        try:
            # read rsa private key file
            with salt.utils.files.fopen(config.get('rsa-private-key'), 'rb') as keyf:
                rsa_pkey = salt.utils.stringutils.to_unicode(keyf.read()).rstrip('\n')
            ssh_key = RSAPrivateKeyAuth(rsa_pkey)
        except Exception as e:
            raise Exception('YaV RSA key configuration error({}).'.format(str(e)))

        vault_client = vault_client_factory(
            rsa_auth=ssh_key,
            rsa_login=config.get('rsa-login'),
        )

    if not vault_client:
        raise Exception('YaV client configuration error. Check if oauth token or login and private key path provided.')

    __context__['yav_client'] = vault_client
    return vault_client


class RedisCacheClient(object):
    def __init__(self, config=None):
        config = config or {}
        self.ttl = 600  # TTL for last version of secret
        self.redis_client = redis.StrictRedis(password=config.get('redis-pass'), socket_timeout=.2)

    def get(self, key):
        try:
            val = self.redis_client.get('secrets/' + key)
            if val is not None:
                val = json.loads(val)
                value = json.loads(val['value'])
                # For the secrets we need to check that timeout is not expired
                if key.startswith('sec'):
                    expired = val['ts'] > time.time() + self.ttl
                else:
                    expired = False
                return value, expired
        except redis.exceptions.RedisError as err:
            # If cache is unavailable - we can look up in the Vault
            log.error(err)
        return None, True

    def put(self, key, value):
        try:
            val = {'value': json.dumps(value), 'ts': int(time.time())}
            self.redis_client.set('secrets/' + key, json.dumps(val))
        except redis.exceptions.RedisError as err:
            log.error(err)


def _get_cache(config):
    if 'cache' in __context__:
        return __context__['cache']
    cache = RedisCacheClient(config)
    __context__['cache'] = cache
    return cache


def _get_value(secret_uuid, config):
    vault_client = _get_yav_client(config)
    cache = _get_cache(config)

    value, expired = cache.get(secret_uuid)
    # Trying to get value from cache
    if value is not None and not expired:
        return value

    try:
        value = vault_client.get_version(secret_uuid, packed_value=False)['value']
        cache.put(secret_uuid, value)
    except ClientError as e:
        # If we don't have stale value in cache - raise exception
        if value is None:
            raise Exception(u'{}: {} (req: {})'.format(
                secret_uuid,
                e.kwargs.get('message'),
                e.kwargs.get('request_id'),
            ))
    return value


def _process_value(value, keys=None):
    packed_value = OrderedDict()
    for v in value:
        processed_value = v['value']
        encoding = v.get('encoding')
        if encoding and encoding == 'base64':
            processed_value = base64.b64decode(processed_value)
        packed_value[v['key']] = processed_value

    if keys:
        result = set()
        val_keys = packed_value.keys()
        for k in keys:
            result.update(fnmatch.filter(val_keys, k))

        packed_value = dict(map(lambda x: (x, packed_value[x]), result))
        if len(keys) == 1:
            return packed_value.get(keys[0])
    return packed_value


def get(data, **kwargs):
    data = ensure_str(data)
    config = _get_config(**kwargs)

    matches = YAV_RE.match(data.strip())
    if not matches:
        return data
    secret_uuid = matches.group('uuid')
    keys = [s.strip() for s in matches.group('keys').split(',')] if matches.group('keys') else None

    value = _get_value(secret_uuid, config)
    return _process_value(value, keys)


def ensure_str(s, encoding='utf-8', errors='strict'):
    """Coerce *s* to `str`.
    For Python 2:
      - `unicode` -> encoded to `str`
      - `str` -> `str`
    For Python 3:
      - `str` -> `str`
      - `bytes` -> decoded to `str`
    NOTE: The function equals to six.ensure_str that is not present in the salt version of six module.
    """
    if type(s) is str:
        return s
    if six.PY2 and isinstance(s, six.text_type):
        return s.encode(encoding, errors)
    elif six.PY3 and isinstance(s, six.binary_type):
        return s.decode(encoding, errors)
    elif not isinstance(s, (six.text_type, six.binary_type)):
        raise TypeError("not expecting type '%s'" % type(s))
    return s
