# coding: utf-8

import base64
import json
import logging
import re

from library.python.vault_client import (
    TokenizedRequest,
    VaultClient,
)
from library.python.vault_client.auth import RSASSHAgentAuth
from library.python.vault_client.errors import (
    ClientError,
    ClientInvalidRsaPrivateKey,
    ClientRsaKeyRequiredPassword,
    ClientSSHAgentError,
    ClientWrappedError,
)
import mock
import paramiko
import pytest
import requests
import requests_mock

from .data import *  # noqa


def get_client(rsa_auth=TEST_RSA_PRIVATE_KEY_1, rsa_login=TEST_RSA_LOGIN):
    adapter = requests_mock.Adapter()
    session = requests.Session()
    session.mount('mock', adapter)
    adapter.register_uri(
        'GET',
        '/status/',
        text=json.dumps({'is_deprecated_client': False, 'status': 'ok', '_mocked': True}),
    )
    adapter.register_uri(
        'GET',
        '/ping.html',
        text='Mocked ping',
    )
    return adapter, VaultClient(
        TEST_URL,
        native_client=session,
        rsa_auth=rsa_auth,
        rsa_login=rsa_login,
        check_status=False,
    )


def mock_create_secret(adapter):
    adapter.register_uri(
        'POST',
        '/1/secrets/',
        text=json.dumps({
            'uuid': 'sec-12345678',
            'status': 'ok',
        })
    )


def mock_create_secret_version(adapter, secret_uuid):
    adapter.register_uri(
        'POST',
        '/1/secrets/%s/versions/' % secret_uuid,
        text=json.dumps({
            'secret_version': 'ver-12345678',
            'status': 'ok',
        })
    )


def mock_get_version(adapter, secret_version, value=None):
    adapter.register_uri(
        'GET',
        '/1/versions/%s/' % secret_version,
        text=json.dumps({
            'version': {
                'created_at': '100',
                'created_by': 1,
                'value': value if value is not None else [{'key': 'password', 'value': '123456'}],
                'version': secret_version,
            },
            'status': 'ok',
        })
    )


def test_client():
    adapter, client = get_client()
    assert client.ping().text == 'Mocked ping'
    assert client.get_status() == {'is_deprecated_client': False, 'status': 'ok', '_mocked': True}

    mock_create_secret(adapter)
    secret_uuid = client.create_secret(name='test-secret')
    mock_create_secret_version(adapter, secret_uuid)
    secret_version = client.create_secret_version(secret_uuid=secret_uuid, value={"password": "123456"})
    mock_get_version(adapter, secret_version)
    retrieved_version = client.get_version(secret_version)
    assert retrieved_version == {
        'created_by': 1,
        'created_at': '100',
        'value': {'password': '123456'},
        'version': 'ver-12345678',
    }


def mock_server_error(adapter, status_code=500, text=u'Internal server error', raise_exception=None):
    if raise_exception is not None:
        adapter.register_uri(
            'GET',
            '/1/versions/ver-12345678/',
            exc=raise_exception,
        )
    else:
        adapter.register_uri(
            'GET',
            '/1/versions/ver-12345678/',
            status_code=status_code,
            text=text,
            headers={
                'Content-Type': 'text/html; charset=utf-8',
            }
        )


def test_server_error():
    adapter, client = get_client()

    mock_server_error(adapter, status_code=200)
    with pytest.raises(ClientError) as excinfo:
        client.get_version('ver-12345678')

    assert excinfo.value.kwargs == {
        'code': 'error',
        'message': 'api failed',
    }

    mock_server_error(adapter, text=u'Internal server errror с русским буковками')
    with pytest.raises(ClientError) as excinfo:
        client.get_version('ver-12345678')

    assert excinfo.value.kwargs == {
        'code': 'error',
        'http_status': 500,
        'message': 'api failed',
        'request_id': client._last_request_id,
        'response_body': u'Internal server errror с русским буковками',
    }

    mock_server_error(adapter, text=u'Internal server errror' + 'ABCD'*1000)
    with pytest.raises(ClientError) as excinfo:
        client.get_version('ver-12345678')

    assert excinfo.value.kwargs == {
        'code': 'error',
        'http_status': 500,
        'message': 'api failed',
        'request_id': client._last_request_id,
        'response_body': 'Internal server errrorABCDABCDABCDABCDABCDABCDABCDABCDABCDABCDABCDABCDABCDABCDABCDABCDABC'
                         'DABCDABCDABCDABCDABCDABCDABCDABCDABCDABCDABCDABCDABCDABCDABCDABCDABCDABCDABCDABCDABCDABCD'
                         'ABCDABCDABCDABCDABCDABCDABCDABCDABCDABCDABCDABCDABCDABCDABCDABCDABCDABCDABCDAB<...>ABCDAB'
                         'CDABCDABCDABCDABCDABCDABCDABCDABCDABCDABCDABCDABCDABCDABCDABCDABCDABCDABCDABCDABCDABCDABC'
                         'DABCDABCDABCDABCDABCDABCDABCDABCDABCDABCDABCDABCDABCDABCDABCDABCDABCDABCDABCDABCDABCDABCD'
                         'ABCDABCDABCDABCDABCDABCDABCDABCDABCDABCDABCDABCDABCDABCDABCDABCDABCDABCD',
    }


def test_send_consumer():
    adapter, client = get_client()
    adapter.register_uri(
        'POST',
        '/1/tokens/',
        text=json.dumps({
            'secrets': [],
            'status': 'ok',
        })
    )
    adapter.register_uri(
        'POST',
        '/1/tokens/revoke/',
        text=json.dumps({
            'result': [],
            'status': 'ok',
        })
    )

    client.send_tokenized_requests(
        [TokenizedRequest(token='token12354'),],
        consumer='vault_client.1.1.2',
    )
    assert adapter.last_request.url == 'mock://vault-api-test.passport.yandex.net/1/tokens/?consumer=vault_client.1.1.2'

    with mock.patch.object(logging.getLogger('vault_client'), 'warning') as warn_mock:
        client.send_tokenized_requests(
            [TokenizedRequest(token='token12354'),],
        )
        assert warn_mock.call_count == 1
        assert re.search(r'consumer must pass the consumer parameter', warn_mock.call_args.args[0]) is not None

    client.send_tokenized_revoke_requests(
        [TokenizedRequest(token='token12354'),],
        consumer='vault_client.1.1.2',
    )
    assert adapter.last_request.url == 'mock://vault-api-test.passport.yandex.net/1/tokens/revoke/?consumer=vault_client.1.1.2'

    with mock.patch.object(logging.getLogger('vault_client'), 'warning') as warn_mock:
        client.send_tokenized_revoke_requests(
            [TokenizedRequest(token='token12354'),],
        )
        assert warn_mock.call_count == 1
        assert re.search(r'consumer must pass the consumer parameter', warn_mock.call_args.args[0]) is not None


def test_catch_requests_errors():
    adapter, client = get_client()
    mock_server_error(adapter, raise_exception=requests.exceptions.ConnectTimeout)

    with pytest.raises(ClientWrappedError) as excinfo:
        client.get_version('ver-12345678')

    assert isinstance(excinfo.value, requests.exceptions.ConnectTimeout)
    assert excinfo.value.kwargs == {'message': 'ConnectTimeout()'}


def b64enc(str):
    return base64.b64encode(str.encode('utf-8')).decode('utf-8')


def test_pack_value():
    unpacked_value = [
        {'encoding': 'base64', 'key': 'login', 'value': b64enc(u'pasha')},
        {'encoding': 'base64', 'key': 'comment', 'value': u''},
        {'key': 'password', 'value': u'123456'},
    ]

    client = VaultClient(TEST_URL, decode_files=True, check_status=False)
    assert client.pack_value(unpacked_value) == {'comment': b'', 'login': b'pasha', 'password': '123456'}
    assert client.pack_value(unpacked_value, decode_files=False) == {'comment': '', 'login': 'cGFzaGE=', 'password': '123456'}

    client = VaultClient(TEST_URL, decode_files=False, check_status=False)
    assert client.pack_value(unpacked_value) == {'comment': '', 'login': 'cGFzaGE=', 'password': '123456'}
    assert client.pack_value(unpacked_value, decode_files=True) == {'comment': b'', 'login': b'pasha', 'password': '123456'}

    adapter, client = get_client()
    secret_version = 'sec-0000000000000000000000ygj0'
    mock_get_version(adapter, secret_version, unpacked_value)
    assert client.get_version(secret_version)['value'] == {'comment': '', 'login': 'cGFzaGE=', 'password': '123456'}
    assert client.get_version(secret_version, decode_files=True)['value'] == {'comment': b'', 'login': b'pasha', 'password': '123456'}


def test_invalid_rsa_key():
    with pytest.raises(ClientInvalidRsaPrivateKey):
        VaultClient(TEST_URL, rsa_auth='invalid key', rsa_login='admin')


def test_encrypted_rsa_key():
    with pytest.raises(ClientRsaKeyRequiredPassword):
        VaultClient(TEST_URL, rsa_auth=TEST_RSA_ENCRYPTED_PRIVATE_KEY, rsa_login='admin')


class FakeSSHAgent(object):
    def get_keys(self):
        raise paramiko.ssh_exception.SSHException('TEST: SSHAgentError')


def test_ssh_agent_errors():
    with pytest.raises(ClientSSHAgentError) as excinfo:
        _, client = get_client(
            rsa_auth=RSASSHAgentAuth(ssh_agent=FakeSSHAgent()),
        )
        client.get_version('ver-12345')
