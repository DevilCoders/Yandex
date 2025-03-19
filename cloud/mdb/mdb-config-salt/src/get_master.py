#!/usr/bin/python2
"""
Looks up master for the minion it is running on.
"""

import json
import logging
import os
import socket
import ssl
import time
import uuid

from six.moves import http_client as httplib

WINDOWS_HOSTNAME_PATH = r'C:\salt\conf\hostname'

if os.name == 'nt':
    SALT_MASTER_OVERRIDE_PATH = r'C:\salt\conf\minion_master_override'
    SALT_MASTER_CACHE_PATH = r'C:\salt\conf\master_fqdn'
    SALT_MASTER_KEY_CACHE_PATH = r'C:\salt\var\cache\salt\master_key.pub'
    DEPLOY_VERSION_PATH = r'C:\salt\conf\deploy_version'
    MDB_DEPLOY_API_HOST_PATH = r'C:\salt\conf\mdb_deploy_api_host'
    CA_PATH = r'C:\Program Files\MdbConfigSalt\allCAs.pem'
else:
    SALT_MASTER_OVERRIDE_PATH = '/etc/salt/minion_master_override'
    SALT_MASTER_CACHE_PATH = '/var/cache/salt/master_fqdn'
    SALT_MASTER_KEY_CACHE_PATH = '/var/cache/salt/master_key.pub'
    DEPLOY_VERSION_PATH = '/etc/yandex/mdb-deploy/deploy_version'
    MDB_DEPLOY_API_HOST_PATH = '/etc/yandex/mdb-deploy/mdb_deploy_api_host'
    CA_PATH = '/opt/yandex/allCAs.pem'


class EmptyURL(Exception):
    pass


def get_deploy_api_hostname():
    with open(MDB_DEPLOY_API_HOST_PATH, 'r') as f:
        host = f.readline().rstrip('\n')
        if len(host) == 0:
            raise EmptyURL('empty mdb deploy api url at {path}'.format(path=MDB_DEPLOY_API_HOST_PATH))
        return host


def _http_conn_internal(host):
    # This is ugly hack due to `ssl` library in old versions (Ubuntu 14.04)
    # has not SSL contexts.
    create_context = getattr(ssl, 'create_default_context', None)
    if create_context:
        ctx = ssl.create_default_context()
        ctx.load_verify_locations(CA_PATH)
        conn = httplib.HTTPSConnection(host, timeout=5, context=ctx)
    else:
        conn = httplib.HTTPSConnection(host, timeout=5)

    return conn


def _get_master_from_cache():
    try:
        with open(SALT_MASTER_CACHE_PATH) as inp:
            return inp.read().strip()
    except IOError:
        return None


def _put_master_in_cache(fqdn):
    try:
        with open(SALT_MASTER_CACHE_PATH, 'w') as out:
            return out.write(fqdn)
    except Exception as exc:
        logging.error('Unable to cache master: %s', repr(exc))


def _get_master_key_from_cache():
    try:
        with open(SALT_MASTER_KEY_CACHE_PATH, 'r') as inp:
            return inp.read().strip()
    except IOError:
        return None


def _put_master_key_in_cache(public_key):
    try:
        with open(SALT_MASTER_KEY_CACHE_PATH, 'w') as out:
            return out.write(public_key)
    except Exception as exc:
        logging.error('Unable to cache master public key: %s', repr(exc))


def _master_v2_internal(fqdn):
    cached_master = _get_master_from_cache()
    try:
        deploy_api_hostname = get_deploy_api_hostname()
    except Exception as exc:
        logging.error('Unable to get deploy api hostname: %s', repr(exc))
        return cached_master

    try:
        conn = _http_conn_internal(deploy_api_hostname)

        rid = str(uuid.uuid4())
        conn.request('GET', '/v1/minions/{fqdn}/master'.format(fqdn=fqdn), headers={'X-Request-Id': rid})
        response = conn.getresponse()

        response_body = response.read()
        response_status = response.status

        conn.close()
        if response_status == 200:
            data = json.loads(response_body)
            master_name = str(data['master'])
            master_public_key = str(data.get('masterPublicKey', ''))
            if master_name != cached_master:
                _put_master_in_cache(master_name)
                if master_public_key:
                    _put_master_key_in_cache(master_public_key)

            # To write key after update (we got cached salt-master and don't have salt-master-key)
            if master_public_key and not _get_master_key_from_cache():
                _put_master_key_in_cache(master_public_key)

            return master_name
    except Exception as exc:
        logging.error('Unable to get master from %s: %s', deploy_api_hostname, repr(exc))

    return cached_master


def _get_fqdn():
    if os.name == 'nt':
        # this file initialized during vm first run with FQDN from compute metadata
        # can't use socket.getfqdn ot socket.gethostname on Windows
        # as it returns only name part without domain
        with open(WINDOWS_HOSTNAME_PATH) as handle:
            return handle.read().strip()
    else:
        return socket.getfqdn()


def master_public_key():
    return _get_master_key_from_cache()


def master():
    """
    Retrieves salt master based on deploy version
    """
    for _ in range(3):
        try:
            fqdn = _get_fqdn()
            if os.path.exists(SALT_MASTER_OVERRIDE_PATH):
                with open(SALT_MASTER_OVERRIDE_PATH, 'r') as override:
                    master_name = override.readline().rstrip('\n')
                    return master_name

            deploy_version = '2'
            if os.path.exists(DEPLOY_VERSION_PATH):
                with open(DEPLOY_VERSION_PATH, 'r') as override:
                    deploy_version = override.readline().rstrip('\n')

            if deploy_version == '2':
                return _master_v2_internal(fqdn)

            raise RuntimeError('Unexpected deploy version: {version}'.format(version=deploy_version))
        except BaseException:
            logging.exception('Unable to get master')
            time.sleep(1)


if __name__ == '__main__':
    print(master())
