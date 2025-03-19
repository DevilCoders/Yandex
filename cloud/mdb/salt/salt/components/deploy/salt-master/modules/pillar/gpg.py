# coding: utf8
import logging
import time
import uuid
from datetime import datetime

from dateutil.parser import parse
from dateutil.tz import tzutc
import requests
import jwt
from nacl.encoding import URLSafeBase64Encoder
from nacl.public import Box, PrivateKey, PublicKey

_GPGNAME = 'gpg'

LOG = logging.getLogger(__name__)

COMPONENTS_ENABLED = set([
    'components.postgres',
    'components.mysql',
    'components.mongodb',
    'components.redis'
])

COMPONENTS_FOREIGN_ENABLED = set([
    'components.walg-server',
])


def __virtual__():
    return _GPGNAME


KEYS_FILES = [
    'pubring.gpg',
    'secring.gpg',
    'trustdb.gpg',
]


class IAMError(Exception):
    pass


def _iam_payload(url, service_account_id, key_id, private_key):
    now = int(time.time())
    payload = {
        'aud': url,
        'iss': service_account_id,
        'iat': now,
        'exp': now + 360,
    }
    return jwt.encode(
        payload,
        private_key.replace("\\n", "\n"),
        algorithm='PS256',
        headers={'kid': key_id},
    )


def _do_request_iam_token(url, encoded_token, ca_path):
    response = requests.post(
        url,
        json={'jwt': encoded_token.decode()},
        verify=ca_path
    )
    if response.status_code != 200:
        raise IAMError('{}: {}'.format(response.status_code, response.text))
    return response.json()


class SACreds(object):
    private_key = None
    service_account_id = None
    key_id = None

    def __init__(self, pk, sa, kid):
        self.private_key = pk
        self.service_account_id = sa
        self.key_id = kid


_CACHED = {
    'token': None,
    'expires': datetime.now(tzutc()),
}


def expired_or_none():
    if not _CACHED['token']:
        return True
    now = datetime.now(tzutc())
    return now >= _CACHED['expires']


def _request_iam_token(url, sa_creds, ca_path):
    """

    @type sa_creds: SACreds
    """
    if expired_or_none():
        LOG.info('not using cache, requesting token from iam %s', url)
        response_json = _do_request_iam_token(
            url=url,
            encoded_token=_iam_payload(
                url=url,
                service_account_id=sa_creds.service_account_id,
                key_id=sa_creds.key_id,
                private_key=sa_creds.private_key
            ),
            ca_path=ca_path
        )
        _CACHED['token'] = response_json['iamToken']
        _CACHED['expires'] = parse(response_json['expiresAt'])
    return _CACHED['token']


def auth_headers_format(oauth=None, iam_token=None):
    if iam_token is not None:
        return 'Bearer {}'.format(iam_token)
    if oauth is not None:
        return 'OAuth %s' % oauth
    raise Exception('no auth present')


def _missing_keys_in_pillar(data):
    return [key for key in KEYS_FILES if key not in data]


def _request_key(name, auth_header, mdb_secrets_key, privkey, url):
    req_id = str(uuid.uuid4())
    request_headers = {
        'Accept': 'application/json',
        'Authorization': auth_header,
        'Content-Type': 'application/json',
        'X-Request-Id': req_id,
    }

    resp = requests.put(
        url,
        headers=request_headers,
        verify='/opt/yandex/allCAs.pem',
        params={'cid': name},
        timeout=20,  # if mdb-secrets needs to generate a key, it may take a long time
    )

    if resp.status_code != 200:
        raise RuntimeError(
            'Unexpected mdb-secrets response for request_id {request_id}: {status_code} {text}'.format(
                request_id=req_id,
                status_code=resp.status_code, text=resp.text))

    return _decrypt(resp.json().get('key'), mdb_secrets_key, privkey)


def _decrypt(encrypted_msg, mdb_secrets_key, privkey):
    """
    Decrypt data using secret key and public key
    """
    version = encrypted_msg['version']
    if version == 1:
        box = Box(
            PrivateKey(privkey.encode('utf-8'), URLSafeBase64Encoder),
            PublicKey(mdb_secrets_key.encode('utf-8'), URLSafeBase64Encoder))
        try:
            return box.decrypt(URLSafeBase64Encoder.decode(
                encrypted_msg['data'].encode('utf-8'))).decode('utf-8')
        except:
            log = logging.getLogger(__name__)
            log.error(encrypted_msg['data'].encode('utf-8'))
            raise
    else:
        raise RuntimeError('Unexpected encryption version: %d' % version)


def _enabled(pillar):
    """
    See if gpg is enabled for this minion/pillar
    """
    data = pillar.get('data', {})
    pillar_has_key = data.get('s3', {}).get('gpg_key', False)
    if pillar_has_key:
        return False

    pillar_has_legacy_keys = not _missing_keys_in_pillar(pillar)
    if pillar_has_legacy_keys:
        return False

    is_dbaas = data.get('dbaas', {}).get('cluster_id', False)
    if is_dbaas:
        return False

    run_list = set(data.get('runlist', []))
    return run_list.intersection(COMPONENTS_ENABLED)


def _pillar_get_path(pillar, path, default=None):
    """
    Find value by `:` separated path in pillar dict
    """
    value = pillar
    for nextkey in path.split(':'):
        value = value.get(nextkey)
        if value is None:
            return default
    return value


def _pillar_set_value(pillar, path, value):
    """
    Set value by `:` separated path in pillar dict
    """
    subpillar = pillar
    pillar_keys = path.split(':')
    for i, nextkey in enumerate(pillar_keys):
        if not nextkey:
            continue
        if not isinstance(subpillar, dict):
            LOG.error('Cannot set pillar value %s - already exists of type %s', path, type(subpillar).__name__)
            return
        if i < len(pillar_keys) - 1:
            if nextkey not in subpillar:
                subpillar[nextkey] = dict()
            subpillar = subpillar[nextkey]
        else:
            # Last iteration, set value
            subpillar[nextkey] = value


def _pillar_path_join(*args):
    return ':'.join(map(lambda x: x.rstrip(':').lstrip(':'), args))


def _foreign_enabled(pillar):
    """
    See if get foreign gpg is enabled for this minion/pillar
    """
    run_list = set(_pillar_get_path(pillar, 'data:runlist', []))
    return run_list.intersection(COMPONENTS_FOREIGN_ENABLED)


def _get_foreign_clusters(pillar):
    """
    Find list of clusters in pillar in following order
    - data:gpg:foreign_clusters_pillar_path
    - data:walg-server:clusters
    - return empty list
    """
    clusters = _pillar_get_path(pillar, 'data:gpg:foreign_clusters_pillar_path')
    if clusters and isinstance(clusters, list):
        return clusters
    clusters = _pillar_get_path(pillar, 'data:walg-server:clusters')
    if clusters and isinstance(clusters, list):
        return clusters
    return list()


def _mdb_secrets_configured(oauth, use_iam_tokens, mdb_secrets_key, privkey):
    return (oauth or use_iam_tokens) and mdb_secrets_key and privkey


def _get_cluster_name(pillar, minion_id):
    cluster_name = pillar.get('data', {}).get('gpg', {}).get('cluster_name')
    if cluster_name:
        return cluster_name
    dbaas_cid = pillar.get('data', {}).get('dbaas', {}).get('cluster_id')
    if dbaas_cid:
        return dbaas_cid
    # Last letter is DC:
    #   katan-db-test01i.db.yandex.net
    return minion_id.split('.')[0][:-1]


def ext_pillar(minion_id, pillar, oauth='', mdb_secrets_key='', privkey='',
               sa_private_key='', sa_id='', sa_key_id='',
               use_iam_tokens=False, tokens_url='',
               url='https://mdb-secrets.db.yandex.net/v1/gpg',
               ca_path='/srv/salt/components/common/conf/allCAs.pem',
               iam_ca_path=None):

    foreign_keys_enabled = _foreign_enabled(pillar)

    if not foreign_keys_enabled and not _enabled(pillar):
        return {}

    if not foreign_keys_enabled and not _mdb_secrets_configured(oauth, use_iam_tokens, mdb_secrets_key, privkey):
        missing_keys = _missing_keys_in_pillar(pillar)
        if missing_keys:
            raise RuntimeError(
                'mdb-secrets disabled for that minion. '
                '%r gpg keys are missing. Fill them in pillar' % missing_keys)
        return {}

    iam_token = None
    if use_iam_tokens:
        sa_creds = SACreds(
            sa_private_key, sa_id, sa_key_id
        )
        iam_token = _request_iam_token(tokens_url, sa_creds, iam_ca_path or ca_path)
    auth_value = auth_headers_format(
        oauth=oauth or None,
        iam_token=iam_token,
    )

    if foreign_keys_enabled:
        keys_path = _pillar_get_path(pillar, 'data:gpg:foreign_clusters_keys_pillar_path', 'data:walg-server:keys')
        result = dict()
        try:
            for cluster in _get_foreign_clusters(pillar):
                key = _request_key(cluster, auth_value, mdb_secrets_key, privkey, url)
                _pillar_set_value(result, _pillar_path_join(keys_path, cluster, 'gpg_key'), key)
                _pillar_set_value(result, _pillar_path_join(keys_path, cluster, 'gpg_key_id'), cluster)
        except Exception as exc:
            LOG.error('gpg gen exception: %s: %s', type(exc).__name__, exc)
            raise
        return result

    name = _get_cluster_name(pillar, minion_id)
    try:
        key = _request_key(name, auth_value, mdb_secrets_key, privkey, url)
        res = {'data': {'s3': {}}}
        res['data']['s3']['gpg_key'] = key
        res['data']['s3']['gpg_key_id'] = name
        return res
    except Exception as exc:
        LOG.error('gpg gen exception: %s: %s', type(exc).__name__, exc)
        raise
