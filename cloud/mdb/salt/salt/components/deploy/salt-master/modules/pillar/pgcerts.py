import logging
import uuid
import time
from datetime import datetime

from dateutil.parser import parse
from dateutil.tz import tzutc
import requests
import jwt
from nacl.encoding import URLSafeBase64Encoder
from nacl.public import Box, PrivateKey, PublicKey


LOG = logging.getLogger(__name__)

COMPONENTS_ENABLED = set([
    'components.postgres',
    'components.mongodb',
    'components.kvm-host',
    'components.clickhouse',
    'components.salt-master',
    'components.deploy.salt-master',
    'components.envoy',
    'components.web-api-base',
    'components.nginx',
    'components.elasticsearch',
    'components.redis',
    'components.kafka',
    'components.zk',
])

CRT = 'cert.crt'
KEY = 'cert.key'
CA = 'cert.ca'
DEFAULT_URL = 'https://mdb-secrets.db.yandex.net/v1/cert'
DEFAULT_CA_PATH = '/srv/salt/components/common/conf/allCAs.pem'
DEFAULT_CERT_TYPE = 'host'


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
        url=url,
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
                url,
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


def _request_mdb_secrets_cert(hostname, auth_header_value, mdb_secrets_key, privkey, url, alt_names, cert_type, readonly):
    req_id = str(uuid.uuid4())
    request_headers = {
        'Accept': 'application/json',
        'Authorization': auth_header_value,
        'Content-Type': 'application/json',
        'X-Request-Id': req_id,
    }

    if readonly:
        resp = requests.get(
            url,
            headers=request_headers,
            verify='/opt/yandex/allCAs.pem',
            params={'hostname': hostname},
            timeout=10,
        )
        if resp.status_code == 200:
            return _parse_resp(resp, mdb_secrets_key, privkey)
        elif resp.status_code == 404:
            LOG.info("certificate not found for %s" % hostname)
            return {}
        else:
            raise RuntimeError(
                'Unexpected mdb-secrets response for request_id {request_id}: {status_code} {text}'.format(
                    request_id=req_id,
                    status_code=resp.status_code, text=resp.text))
    else:
        params = {
            'hostname': hostname,
            'ca': 'InternalCA',  # this is not needed in Cloud Crt
            'type': cert_type,
        }
        if alt_names:
            params['alt_names'] = ','.join(alt_names)
        resp = requests.put(
            url,
            headers=request_headers,
            verify='/opt/yandex/allCAs.pem',
            params=params,
            timeout=600,  # on first call when cert issued dns challenge can take more than 300(dns ttl)
        )

        if resp.status_code != 200:
            raise RuntimeError(
                'Unexpected mdb-secrets response for request_id {request_id}: {status_code} {text}'.format(
                    request_id=req_id,
                    status_code=resp.status_code, text=resp.text))
        return _parse_resp(resp, mdb_secrets_key, privkey)


def _parse_resp(resp, mdb_secrets_key, privkey):
    resp_json = resp.json()
    return {
        CRT: resp_json['cert'],
        KEY: _decrypt(resp_json['key'], mdb_secrets_key, privkey),
    }


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
            LOG.error(encrypted_msg['data'].encode('utf-8'))
            raise
    else:
        raise RuntimeError('Unexpected encryption version: %d' % version)


def ext_pillar(
        minion_id, pillar,
        sa_private_key='', sa_id='', sa_key_id='',
        use_iam_tokens=False, tokens_url='',
        mdb_secrets_key='', privkey='', mdb_secrets_oauth='',
        url=DEFAULT_URL,
        ca_path=DEFAULT_CA_PATH,
        iam_ca_path=None,
        cert_type=DEFAULT_CERT_TYPE,
        wildcard_hostname=False):
    """
    Certificates ext_pillar
    """
    def _with_ca(into_ret):
        # Fill cert.ca if it not present
        if CA not in pillar:
            with open(ca_path) as ca_fd:
                into_ret[CA] = ca_fd.read()
        return into_ret

    # Immediately return if certs are already in pillar
    if KEY in pillar and CRT in pillar:
        return _with_ca({})

    # return only cert.ca for pg-barman
    run_list = set(pillar.get('data', {}).get('runlist', []))

    if not (pillar.get('data', {}).get('use_mdbsecrets', False) or
            run_list.intersection(COMPONENTS_ENABLED)):
        return {}

    hostname = pillar.get('data', {}).get('pg_ssl_balancer', minion_id)
    alt_names = pillar.get('data', {}).get('pg_ssl_balancer_alt_names', [])
    cert_type = pillar.get('data', {}).get('certs', {}).get('type', cert_type)

    if wildcard_hostname:
        #  Convert ach-ec1c-s1-1.cid1.yadc.io => *.cid1.yadc.io
        hostname = '.'.join(['*'] + hostname.split('.')[1:])

    if not _is_mdb_secrets_configured(mdb_secrets_key, privkey, mdb_secrets_oauth, use_iam_tokens, url):
        raise RuntimeError(
            "cannot use mdb-secrets for that minion. "
            "Fill certificates in pillar (toplevel %r) or properly configure pgcerts ext_pillar" % [KEY, CRT]
        )
    iam_token = None
    if use_iam_tokens:
        sa_creds = SACreds(
            sa_private_key, sa_id, sa_key_id
        )
        iam_token = _request_iam_token(tokens_url, sa_creds, iam_ca_path)
    auth_value = auth_headers_format(
        oauth=mdb_secrets_oauth or None,
        iam_token=iam_token,
    )

    readonly = pillar.get('data', {}).get('certs', {}).get('readonly', False)

    ret = _request_mdb_secrets_cert(hostname, auth_value, mdb_secrets_key, privkey, url, alt_names, cert_type, readonly)

    return _with_ca(ret)


def _is_mdb_secrets_configured(mdb_secrets_key, privkey, mdb_secrets_oauth, use_iam_tokens, url):
    return mdb_secrets_key and privkey and (mdb_secrets_oauth or use_iam_tokens) and url


def _main():
    import argparse
    import yaml
    parser = argparse.ArgumentParser()
    parser.add_argument('hostname')
    parser.add_argument('--alt-names', nargs='*')
    parser.add_argument('--cert-type', default=DEFAULT_CERT_TYPE)

    args = parser.parse_args()

    secrets_config = {}
    with open('/etc/salt/ext_pillars') as fd:
        for cfg_item in yaml.safe_load(fd)['ext_pillar']:
            secrets_config = cfg_item.get('pgcerts')
            if secrets_config:
                break
        else:
            raise RuntimeError('pgcerts section not found in ext_pillar config')

    ca_path = secrets_config['ca_path'] or DEFAULT_CA_PATH

    iam_token = None
    if secrets_config['use_iam_tokens']:
        sa_creds = SACreds(
            secrets_config['sa_private_key'], secrets_config['sa_id'], secrets_config['sa_key_id'],
        )
        iam_token = _request_iam_token(
            secrets_config['tokens_url'],
            sa_creds,
            secrets_config.get('iam_ca_path') or ca_path)

    auth_value = auth_headers_format(
        oauth=secrets_config.get('mdb_secrets_oauth') or None,
        iam_token=iam_token,
    )
    new_cert = _request_mdb_secrets_cert(
        args.hostname,
        auth_value,
        secrets_config['mdb_secrets_key'],
        secrets_config['privkey'],
        secrets_config['url'] or DEFAULT_CA_PATH,
        args.alt_names or [],
        args.cert_type,
        False,
    )
    for key, value in new_cert.items():
        print(key)
        print(value)


if __name__ == '__main__':
    _main()
