# -*- coding: utf-8 -*-

import base64
import getpass
import json
import logging
import os
import platform
import sys
import time

import pkg_resources
import requests
import six
from six.moves import urllib

from . import __version__ as vault_client_version
from .auth import (
    BaseRSAAuth,
    RSAPrivateKeyAuth,
    RSASSHAgentAuth,
)
from .errors import (
    ClientError,
    ClientRsaKeysNotFound,
    ClientUnknownRSAAuthType,
    wrap_unknown_errors,
)
from .utils import (
    default_hash,
    merge_dicts,
    noneless_dict,
    sign_in_memory,
    token_hex,
)


try:
    from json.decoder import JSONDecodeError
except ImportError:
    JSONDecodeError = ValueError

VAULT_CLIENT_HTTP_TIMEOUT = 5  # http-timeout in seconds
YANDEX_INTERNAL_ROOT_CA_CERT_FILENAME = pkg_resources.resource_filename(__name__, 'YandexInternalRootCA.crt')

logger = logging.getLogger('vault_client')
ch = logging.StreamHandler(sys.stderr)
ch.setLevel(logging.WARNING)
logger.addHandler(ch)


class _InvalidRSASignature(Exception):
    pass


class TokenizedRequest(dict):
    def __init__(self, token, secret_version=None, signature=None, service_ticket=None, secret_uuid=None, uid=None):
        super(TokenizedRequest, self).__init__(
            token=token,
            secret_version=secret_version,
            signature=signature,
            service_ticket=service_ticket,
            secret_uuid=secret_uuid,
            uid=uid,
        )


class VaultClient(object):
    """
    User authorization is conducting by TVM2.0 user ticker, SSH private keys or OAuth tokens.
    SSH validation conductes through signature validation by public keys retrieved from https://staff.yandex-team.ru/
    """
    def __init__(self, host='', native_client=None, ca_cert=None, timeout=VAULT_CLIENT_HTTP_TIMEOUT,
                 user_ticket=None, rsa_auth=True, rsa_login=None, authorization=None, service_ticket=None,
                 decode_files=False, check_status=False):
        """
        user_ticket: String with TVM2.0 user ticket
        service_ticket: String with TVM 2.0 service ticket
        rsa_auth: If set to True request will be signed by the first private key from SSH Agent.
            Also, required private key can be passed by string.
        rsa_login: User login, defaulted to system username. Required for retrieving public keys from Staff
        authorization: OAuth token
        decode_files: Automatically decode files stored in secrets. Parameter is going to be set to True by default
            in the next major release. Please use decode_files=True by default.
        """
        self.host = host
        self.native_client = native_client
        if not self.native_client:
            self.native_client = self.create_native_client(ca_cert)

        self.timeout = timeout

        self.user_ticket = user_ticket
        self.service_ticket = service_ticket

        self.rsa_auth = self._build_rsa_auth(rsa_auth, rsa_login)
        self.rsa_login = rsa_login
        self.authorization = authorization

        self.user_agent = 'YandexVaultClient/{version} ({system}; {platform}; {node}) {p_impl}/{p_ver} ({p_comp})'.format(
            version=VaultClient.get_client_version(),
            system=platform.system(),
            platform=platform.platform(),
            node=platform.node(),
            p_impl=platform.python_implementation(),
            p_ver=platform.python_version(),
            p_comp=platform.python_compiler(),
        )

        self._request_id_prefix = token_hex(8)
        self._last_request_id = None

        self.decode_files = decode_files

        if check_status:
            self.check_status()

    @staticmethod
    def get_client_version():
        return vault_client_version

    @staticmethod
    def create_native_client(ca_cert=None):
        if ca_cert is None:
            if os.path.exists(YANDEX_INTERNAL_ROOT_CA_CERT_FILENAME):
                ca_cert = YANDEX_INTERNAL_ROOT_CA_CERT_FILENAME
            else:
                ca_cert = True
        native_client = requests.Session()
        native_client.verify = ca_cert
        return native_client

    def validate_response(self, response):
        try:
            data = response.json()
        except JSONDecodeError:
            ex_data = dict(
                code='error',
                message='api failed',
            )
            if response.status_code >= 500:
                ex_data['http_status'] = response.status_code
                ex_data['request_id'] = self._last_request_id

                ex_data['response_body'] = response.text
                if len(response.text) > 512:
                    ex_data['response_body'] = response.text[:256] + '<...>' + response.text[-256:]
            raise ClientError(**ex_data)
        if response.status_code < 200 or response.status_code >= 300 or data.get('status') == 'error':
            raise ClientError(request_id=data.get('api_request_id') or self._last_request_id, **data)
        if data.get('status') == 'warning':
            default_message = 'request %s is somehow wrong' % data.get('request_id', '')
            logger.warning('Warning: %s' % data.get('warning_message', default_message))
        return data

    def unpack_value(self, value):
        if isinstance(value, (tuple, list)):
            return value
        if isinstance(value, dict):
            real_value = []
            for k, v in value.items():
                real_value.append({'key': k, 'value': v})
            return real_value
        raise ClientError(message='Unknown value format')

    def pack_value(self, unpacked_value, decode_files=None):
        value = {}
        skip_warning = decode_files is not None
        decode_files = decode_files if decode_files is not None else self.decode_files
        has_files = False
        for el in unpacked_value:
            if el.get('encoding') == 'base64':
                has_files = True
                if decode_files:
                    value[el['key']] = base64.b64decode(el['value'].encode('utf-8'))
                    continue
            value[el['key']] = el['value']

        if not skip_warning and has_files and not decode_files:
            logger.warning(
                'Warning: you use secrets version with files without decoding file value. '
                'Add decode_files=True to the VaultClient constructor'
            )
        return value

    def _headers(self, service_ticket=None, user_ticket=None, authorization=None, content_type=None):
        self._last_request_id = self._request_id_prefix + token_hex(8)
        return noneless_dict({
            'Authorization': authorization,
            'Content-Type': content_type,
            'User-Agent': self.user_agent,
            'X-Request-Id': self._last_request_id,
            'X-Ya-Service-Ticket': service_ticket,
            'X-Ya-User-Ticket': user_ticket,
        })

    def _serialize_request(self, method, path, data, timestamp, login):
        r = u'%s\n%s\n%s\n%s\n%s\n' % (method.upper(), path, data, timestamp, login)
        return r.encode('utf-8')

    def _proceed_base_auth(self, method, path, data=None, headers=None, skip_headers=False):
        ctx = dict()
        ctx['headers'] = self._headers(**merge_dicts(
            dict(
                authorization=self.authorization,
                service_ticket=self.service_ticket,
                user_ticket=self.user_ticket,
            ),
            headers if not skip_headers else {},
        ))
        yield ctx

    def _build_rsa_auth(self, rsa_auth=False, rsa_login=None):
        if not rsa_auth:
            return None

        if isinstance(rsa_auth, BaseRSAAuth):
            return rsa_auth
        elif type(rsa_auth) is int or rsa_auth is None or (type(rsa_auth) is bool and rsa_auth is True):
            return RSASSHAgentAuth(key_num=rsa_auth)
        elif isinstance(rsa_auth, six.string_types):
            if not rsa_login:
                logger.warning(
                    'WARNING! RSA private key passed without explicit passing rsa_login. '
                    'Your system login `%s` will be used for request signing' % getpass.getuser()
                )  # pragma: no cover
            return RSAPrivateKeyAuth(rsa_auth)
        else:
            raise ClientUnknownRSAAuthType()

    def _rsa_sign_data(self, data, key):
        # Версия подписи: 3 (VAULT-151)
        # Подписываем хеш от сериализованного запроса, чтобы не упереться в ограничение
        # на длину подписываемых данных в некоторых ssh-агентах
        return sign_in_memory(default_hash(data), key=key)

    def _proceed_rsa_auth(self, method, path, data=None, headers=None, skip_headers=False):
        rsa_login = self.rsa_login
        headers = self._headers(**(headers or {}))
        ctx = dict(headers=headers if not skip_headers else {})

        rsa_keys = self.rsa_auth()
        if not rsa_keys:
            raise ClientRsaKeysNotFound()

        if rsa_login is None:
            rsa_login = os.environ.get('SUDO_USER') or getpass.getuser()

        for key in rsa_keys:
            timestamp = str(int(time.time()))
            signature = self._rsa_sign_data(
                self._serialize_request(method, path, data, timestamp, rsa_login),
                key=key,
            )

            ctx['headers']['X-Ya-Rsa-Signature'] = signature
            ctx['headers']['X-Ya-Rsa-Login'] = rsa_login
            ctx['headers']['X-Ya-Rsa-Timestamp'] = timestamp

            yield ctx

    def _validate_rsa_native_response(self, response):
        if response.status_code == 401:
            data = response.json()
            if data.get('code') == 'rsa_signature_error':
                raise _InvalidRSASignature()

    @wrap_unknown_errors
    def _call_native_client(self, method, url, params=None, data=None, headers=None,
                            skip_headers=False, timeout=None, skip_auth=False,
                            send_service_ticket_only=False):
        response = None
        client_func = getattr(self.native_client, method)
        path = url.rstrip('?') + '?'
        headers = dict(headers) if headers else {}

        params = dict(params) if params else {}
        if params:
            path = path + urllib.parse.urlencode(params)

        if data is None:
            data = ''
        elif isinstance(data, dict):
            data = json.dumps(data)
            headers['content_type'] = 'application/json'

        if send_service_ticket_only:
            headers['service_ticket'] = self.service_ticket

        if skip_auth or send_service_ticket_only:
            return client_func(
                path,
                data=data,
                headers=self._headers(**(headers or {})),
                timeout=timeout or self.timeout,
            )

        if self.user_ticket or self.authorization or not self.rsa_auth:
            for ctx in self._proceed_base_auth(
                method=method,
                path=path,
                data=data,
                headers=headers,
                skip_headers=skip_headers,
            ):
                return client_func(
                    path,
                    data=data,
                    headers=ctx.get('headers'),
                    timeout=timeout or self.timeout,
                )

        for ctx in self._proceed_rsa_auth(
            method=method,
            path=path,
            data=data,
            headers=headers,
            skip_headers=skip_headers,
        ):
            try:
                response = client_func(
                    path,
                    params=params,
                    data=data,
                    headers=ctx.get('headers'),
                    timeout=timeout or self.timeout,
                )
                self._validate_rsa_native_response(response)
                return response
            except _InvalidRSASignature:
                pass
        return response

    def ping(self):
        r = self._call_native_client(
            'get',
            self.host + '/ping.html',
            skip_auth=True,
        )
        return r

    def get_status(self):
        """Get server status.

        Returns:
            Dict with status data

        Raises:
            ClientError: An error occurred doing request
        """
        r = self._call_native_client('get', self.host + '/status/', skip_auth=True)
        data = self.validate_response(r)
        return data

    def check_status(self):
        """Check server status.

        Returns:
            True if OK, print warning for outdated clients

        Raises:
            ClientError: An error occurred doing request
        """
        data = self.get_status()
        if data['is_deprecated_client']:
            logger.warning(
                'DeprecationWarning: please, update your Vault client ({version}) as soon as possible!'.format(
                    version=self.get_client_version(),
                ),
            )
        return True

    def list_secrets(self, tags=None, order_by=None, asc=False, role=None, yours=False,
                     query=None, query_type=None,
                     with_hidden_secrets=False, with_tvm_apps=False,
                     page=None, page_size=None, return_raw=False):
        """List of secrets (without value).

        Args:
            tags: comma-separated list of tags
            order_by: order field
            asc: True if ascending order required, False otherwise
            role: role for filtering
            yours: True if you want your secrets, False otherwise
            query: Search query
            query_type: Search type ('infix', 'language', 'exact', 'prefix')
                        infix - searches for a substring
                        language - using MySQL full-text search (natural mode)
                        exact — searches for an exact match of a string (case-sensitive)
                        prefix — searches for a substring at the beginning of a string
            with_hidden_secrets: True if hidden secrets requires, False otherwise
            with_tvm_apps: True to get secrets tvm apps
            page: Page number
            page_size: page size
            return_raw: If set to True raw response will be returned instead of processed one

        Returns:
            A list of secrets or raw response depending on return_raw parameter.

        Raises:
            ClientError: An error occurred doing request
        """
        url = self.host + '/1/secrets/'
        request_data = noneless_dict({
            'tags': tags,
            'order_by': order_by,
            'asc': asc,
            'role': role,
            'yours': yours,
            'query': query,
            'query_type': query_type,
            'with_hidden_secrets': with_hidden_secrets,
            'with_tvm_apps': with_tvm_apps,
            'page': page,
            'page_size': page_size,
        })

        r = self._call_native_client('get', url, data=request_data)
        if return_raw:
            return r  # pragma: no cover

        data = self.validate_response(r)
        return data['secrets']

    def create_secret(self, name, comment=None, tags=None, return_raw=False):
        """Create new secret.

        Create new secret authorizing user by TVM2.0 user ticker or SSH private keys.
        SSH validation conducted through signature validation by public keys retrieved
        from https://staff.yandex-team.ru/

        Args:
            name: Non unique secret name
            comment: Secret comment
            tags: comma-separated list of tags
            return_raw: If set to True raw response will be returned instead of processed one

        Returns:
            Secret UUID or raw response depending on return_raw parameter.

        Raises:
            ClientError: An error occurred doing request
        """
        url = self.host + '/1/secrets/'

        request_data = noneless_dict({'name': name, 'comment': comment, 'tags': tags})

        r = self._call_native_client('post', url, data=request_data)
        if return_raw:
            return r  # pragma: no cover

        data = self.validate_response(r)
        return data['uuid']

    def get_secret(self, secret_uuid, page=None, page_size=None, return_raw=False):
        """Get secret meta information.

        Args:
            secret_uuid: Secret UUID
            page: Page number
            page_size: page size
            return_raw: If set to True raw response will be returned instead of processed one

        Returns:
            A dict with secret meta information or raw response depending on return_raw parameter.

        Raises:
            ClientError: An error occurred doing request
        """
        url = self.host + '/1/secrets/%s/' % secret_uuid
        request_data = noneless_dict({'page': page, 'page_size': page_size})

        r = self._call_native_client('get', url, data=request_data)
        if return_raw:
            return r  # pragma: no cover

        data = self.validate_response(r)
        return data['secret']

    def update_secret(self, secret_uuid, name=None, comment=None, tags=None,
                      state=None, return_raw=False):
        """Update secret metadata.

        Args:
            secret_uuid: Secret UUID
            name: New secret name
            comment: New secret comment
            tags: comma-separated list of tags
            state: state, one of 'normal' or 'hidden'
            return_raw: If set to True raw response will be returned instead of processed one

        Returns:
            True if OK or raw response depending on return_raw parameter.

        Raises:
            ClientError: An error occurred doing request
        """
        url = self.host + '/1/secrets/%s/' % secret_uuid
        request_data = noneless_dict({'name': name, 'comment': comment, 'tags': tags, 'state': state})

        r = self._call_native_client('post', url, data=request_data)
        if return_raw:
            return r  # pragma: no cover

        self.validate_response(r)
        return True

    def create_secret_version(self, secret_uuid, value, ttl=None, comment=None, return_raw=False):
        """Create new secret version.

        Args:
            secret_uuid: Secret UUID
            value: Secret value, can be in packed format (i.e. {"username": "ppodolsky", "password": "123456"}) or
                in native format (i.e. [{"key": "username", "value": "ppodolsky"},
                {"key": "password", "value": "123456"}])
            ttl: secret lifetime in seconds
            comment: comment
            return_raw: If set to True raw response will be returned instead of processed one

        Returns:
            Secret version UUID or raw response depending on return_raw parameter.

        Raises:
            ClientError: An error occurred doing request
        """
        url = self.host + '/1/secrets/%s/versions/' % secret_uuid
        request_data = noneless_dict({
            'value': self.unpack_value(value),
            'ttl': ttl,
            'comment': comment,
        })

        r = self._call_native_client('post', url, data=request_data)
        if return_raw:
            return r  # pragma: no cover

        data = self.validate_response(r)
        return data['secret_version']

    def create_diff_version(self, parent_version_uuid, diff, ttl=None, comment=None,
                            check_head=None, return_raw=False):
        """Create new secret version from the old one

        Args:
            parent_version_uuid: version, secret or bundle uuid
            diff: Secret value diff in the following format: [{'key': 'a', 'value': 1}, {'key': 'b'}] meaning
            'a' will be updated to value 1 and 'b' will be removed
            comment: comment
            check_head: throw error if parent version is not equal to head version
            return_raw: If set to True raw response will be returned instead of processed one

        Returns:
            Secret version UUID or raw response depending on return_raw parameter.

        Raises:
            ClientError: An error occurred doing request
        """
        url = self.host + '/1/versions/%s/' % parent_version_uuid
        request_data = noneless_dict({
            'diff': diff,
            'ttl': ttl,
            'comment': comment,
            'check_head': 'true' if check_head else None,
        })

        r = self._call_native_client('post', url, data=request_data)
        if return_raw:
            return r  # pragma: no cover

        data = self.validate_response(r)
        return data['version']

    def get_version(self, version, packed_value=True, decode_files=None, return_raw=False):
        """Get secret value by user authorization

        Args:
            version: Version or secret uuid. For secret uuid returns head-version
            packed_value: If set to True, value will be transformed into packed format (
            i.e. {"username": "ppodolsky", "password": "123456"}), otherwise it will be
            in native format (i.e. [{"key": "username", "value": "ppodolsky"}, {"key": "password", "value": "123456"}])
            return_raw: If set to True raw response will be returned instead of processed one

        Returns:
            A version data or raw response depending on return_raw parameter.

        Raises:
            ClientError: An error occurred doing request
        """
        url = self.host + '/1/versions/%s/' % version

        r = self._call_native_client('get', url)
        if return_raw:
            return r  # pragma: no cover

        data = self.validate_response(r)
        data = data['version']
        if packed_value:
            if version[:3] in ['sec', 'ver']:
                data['value'] = self.pack_value(data['value'], decode_files=decode_files)
            if version[:3] in ['bun', 'bve']:
                for secret_version in data['secret_versions']:
                    secret_version['value'] = self.pack_value(secret_version['value'], decode_files=decode_files)
        return data

    def update_version(self, version, state=None, ttl=None, comment=None, return_raw=False):
        """Update secret version metadata

        Args:
            version:
            state: state, one of 'normal' or 'hidden'
            ttl: secret lifetime in seconds. 0 — reset ttl
            comment: comment
            return_raw: If set to True raw response will be returned instead of processed one

        Returns:
            An operation status (bool) or raw response depending on return_raw parameter.

        Raises:
            ClientError: An error occurred doing request
        """
        url = self.host + '/1/versions/%s/' % version
        data = noneless_dict({
            'state': state,
            'ttl': ttl,
            'comment': comment,
        })

        r = self._call_native_client('patch', url, data=data)
        if return_raw:
            return r  # pragma: no cover

        self.validate_response(r)
        return True

    def add_user_role_to_secret(self, secret_uuid, role, abc_id=None, abc_scope=None, abc_role_id=None,
                                staff_id=None, uid=None, login=None, return_raw=False):
        """Add user role to secret

        Secret role can be added to user, ABC group or Staff group.

        Args:
            secret_uuid: Secret UUID
            role: Role name, can be one of OWNER or READER or APPENDER
            abc_id: ABC ID of target group
            abc_scope: ABC scope (group name)
            abc_role_id: ABC role ID
            staff_id: Staff ID of target group
            uid: Passport UID of target user
            login: or his yandex login
            return_raw: If set to True raw response will be returned instead of processed one

        Returns:
            True if OK or raw response depending on return_raw parameter.

        Raises:
            ClientError: An error occurred doing request
        """
        url = self.host + '/1/secrets/%s/roles/' % secret_uuid
        request_data = noneless_dict({
            'abc_id': abc_id,
            'abc_scope': abc_scope,
            'abc_role_id': abc_role_id,
            'staff_id': staff_id,
            'uid': uid,
            'role': role.upper(),
            'login': login,
        })

        r = self._call_native_client('post', url, data=request_data)
        if return_raw:
            return r  # pragma: no cover

        self.validate_response(r)
        return True

    def delete_user_role_from_secret(self, secret_uuid, role, abc_id=None, abc_scope=None, abc_role_id=None,
                                     staff_id=None, uid=None, login=None, return_raw=False):
        """Remove user role from secret

        Secret role can be removed from user, ABC group or Staff group. Raises exception if no referenced role exists

        Args:
            secret_uuid: Secret UUID
            role: Role name, can be one of OWNER or READER or APPENDER
            abc_id: ABC ID of target group
            abc_scope: ABC scope (group name)
            abc_role_id: ABC role ID
            staff_id: Staff ID of target group
            uid: Passport UID of target user
            login: or his yandex login
            return_raw: If set to True raw response will be returned instead of processed one

        Returns:
            True if OK or raw response depending on return_raw parameter.

        Raises:
            ClientError: An error occurred doing request
        """
        url = self.host + '/1/secrets/%s/roles/' % secret_uuid
        request_data = noneless_dict({
            'abc_id': abc_id,
            'abc_scope': abc_scope,
            'abc_role_id': abc_role_id,
            'staff_id': staff_id,
            'uid': uid,
            'role': role.upper(),
            'login': login,
        })

        r = self._call_native_client('delete', url, data=request_data)
        if return_raw:
            return r  # pragma: no cover

        self.validate_response(r)
        return True

    def get_owners(self, secret_uuid, return_raw=False):
        """Get a list of owners of secret

        Args:
            secret_uuid: Secret UUID
            return_raw: If set to True raw response will be returned instead of processed one

        Returns:
            A list of owners of secret or raw response depending on return_raw parameter.

        Raises:
            ClientError: An error occurred doing request
        """
        url = self.host + '/1/secrets/%s/owners/' % secret_uuid

        r = self._call_native_client('get', url, skip_auth=True)
        if return_raw:
            return r  # pragma: no cover

        data = self.validate_response(r)
        return data['owners']

    def get_readers(self, secret_uuid, return_raw=False):
        """Get a list of readers of secret

        Args:
            secret_uuid: Secret UUID
            return_raw: If set to True raw response will be returned instead of processed one

        Returns:
            A list of readers of secret or raw response depending on return_raw parameter.

        Raises:
            ClientError: An error occurred doing request
        """
        url = self.host + '/1/secrets/%s/readers/' % secret_uuid

        r = self._call_native_client('get', url, skip_auth=True)
        if return_raw:
            return r  # pragma: no cover

        data = self.validate_response(r)
        return data['readers']

    def get_writers(self, secret_uuid, return_raw=False):
        """Get a list of writers of secret (owners and appenders)

        Args:
            secret_uuid: Secret UUID
            return_raw: If set to True raw response will be returned instead of processed one

        Returns:
            A list of writers of secret or raw response depending on return_raw parameter.

        Raises:
            ClientError: An error occurred doing request
        """
        url = self.host + '/1/secrets/%s/writers/' % secret_uuid

        r = self._call_native_client('get', url, skip_auth=True)
        if return_raw:
            return r  # pragma: no cover

        data = self.validate_response(r)
        return data['writers']

    def can_user_read_secret(self, secret_uuid, uid, return_raw=False):
        """Verify that the user has access to the secret

        Args:
            secret_uuid: Secret UUID
            uid: Passport user ID
            return_raw: If set to True raw response will be returned instead of processed one

        Returns:
            Bool or raw response depending on return_raw parameter.

        Raises:
            ClientError: An error occurred doing request
        """
        url = self.host + '/1/secrets/%s/readers/%s/' % (secret_uuid, uid)

        r = self._call_native_client('get', url, skip_auth=True)
        if return_raw:
            return r  # pragma: no cover

        data = self.validate_response(r)
        return data['access'] == 'allowed'

    def add_supervisor(self, uid, return_raw=False):
        url = self.host + '/1/supervisors/'
        request_data = noneless_dict({'uid': uid})

        r = self._call_native_client('post', url, data=request_data)
        if return_raw:
            return r  # pragma: no cover

        self.validate_response(r)
        return True

    def list_tokens(self, secret_uuid, with_revoked=False, page=None, page_size=None, return_raw=False):
        """List of tokens' information

        Args:
            secret_uuid: Secret UUID
            with_revoked: Add revoked tokens to the list
            return_raw: If set to True raw response will be returned instead of processed one

        Returns:
            List of tokens' information or raw response depending on return_raw parameter.

        Raises:
            ClientError: An error occurred doing request
        """
        url = self.host + '/1/secrets/%s/tokens/' % secret_uuid
        request_data = noneless_dict(dict(
            with_revoked=with_revoked,
            page=page,
            page_size=page_size,
        ))

        r = self._call_native_client('get', url, data=request_data)
        if return_raw:
            return r  # pragma: no cover

        data = self.validate_response(r)
        return data['tokens']

    def create_token(self, secret_uuid, tvm_client_id=None, signature=None, comment=None,
                     return_raw=False):
        """Create new token

        Args:
            secret_uuid: Secret UUID
            tvm_client_id: TVM Client ID binded to token
            signature: second factor of authorization
            return_raw: If set to True raw response will be returned instead of processed one

        Returns:
            token and token_uuid or raw response depending on return_raw parameter.

        Raises:
            ClientError: An error occurred doing request
        """
        url = self.host + '/1/secrets/%s/tokens/' % secret_uuid
        request_data = noneless_dict({
            'tvm_client_id': tvm_client_id,
            'signature': signature,
            'comment': comment,
        })

        r = self._call_native_client('post', url, data=request_data)
        if return_raw:
            return r  # pragma: no cover

        data = self.validate_response(r)
        return data['token'], data.get('token_uuid')

    def get_token_info(self, token_uuid=None, token=None, return_raw=False):
        """Get token info by token_uuid or token

        Args:
            token_uuid: Token UUID
            token: Delegation token
            return_raw: If set to True raw response will be returned instead of processed one

        Returns:
            dict(token_info, owners) or raw response depending on return_raw parameter.

        Raises:
            ClientError: An error occurred doing request
        """
        url = self.host + '/1/tokens/info/'
        request_data = noneless_dict({
            'token_uuid': token_uuid,
            'token': token,
        })

        r = self._call_native_client('post', url, data=request_data)
        if return_raw:
            return r  # pragma: no cover

        data = self.validate_response(r)
        return noneless_dict(dict(
            token_info=data['token_info'],
            owners=data['owners'],
            readers=data.get('readers'),
        ))

    def revoke_token(self, token_uuid, return_raw=False):
        """Revoke token by token_uuid

        Args:
            token_uuid: Token UUID
            return_raw: If set to True raw response will be returned instead of processed one

        Returns:
            Boolean or raw response depending on return_raw parameter.

        Raises:
            ClientError: An error occurred doing request
        """
        url = self.host + '/1/tokens/%s/revoke/' % token_uuid

        r = self._call_native_client('post', url)
        if return_raw:
            return r  # pragma: no cover

        self.validate_response(r)
        return True

    def restore_token(self, token_uuid, return_raw=False):
        """Restore revoked token by token_uuid

        Args:
            token_uuid: Token UUID
            return_raw: If set to True raw response will be returned instead of processed one

        Returns:
            Boolean or raw response depending on return_raw parameter.

        Raises:
            ClientError: An error occurred doing request
        """
        url = self.host + '/1/tokens/%s/restore/' % token_uuid

        r = self._call_native_client('post', url)
        if return_raw:
            return r  # pragma: no cover

        self.validate_response(r)
        return True

    def send_tokenized_requests(self, tokenized_requests, consumer=None, return_raw=False):
        """Get secret value by token

        This method is used to apply token and retrieve any version of secret.

        Args:
            tokenized_requests: Tokenized requests containing tokens,
            if the secret_version parameter is omitted, return head version,
            i.e
                [{
                    'token': 'M40dbFJAFw-_An4TER_gJzPVmtyavr6xNxOsIo-d6q0',
                    'secret_version': 'ver-01ch15v3nengynv145w7cg2vs0',
                    'signature': '123',
                }]
            consumer: The consumer must pass the consumer parameter containing A. B to the handle,
                      where A is the consumer's name, which is agreed together with the quota
                      for the handle, B is the version of the binary that will go into the handle.
            return_raw: If set to True raw response will be returned instead of processed one

        Returns:
            List of secrets values or raw response depending on return_raw parameter.

        Raises:
            ClientError: An error occurred doing request
        """
        url = self.host + '/1/tokens/'
        request_data = noneless_dict({
            'tokenized_requests': tokenized_requests,
        })

        params = dict()
        if consumer is not None:
            params['consumer'] = consumer
        else:
            logger.warning(
                'send_tokenized_requests: The consumer must pass the consumer parameter containing A. B to the handle, '
                'where A is the consumer\'s name, which is agreed together with the quota '
                'for the handle, B is the version of the binary that will go into the handle.'
            )

        r = self._call_native_client(
            'post', url, data=request_data,
            send_service_ticket_only=True,
            params=params,
        )
        if return_raw:
            return r  # pragma: no cover

        data = self.validate_response(r)
        return data['secrets']

    def send_tokenized_revoke_requests(self, tokenized_requests, consumer=None, return_raw=False):
        """Revoke tokens on behalf of the tvm-application

        Args:
            tokenized_requests: Tokenized requests containing tokens,
            i.e
                [{
                    'token': 'M40dbFJAFw-_An4TER_gJzPVmtyavr6xNxOsIo-d6q0',
                    'signature': '123',
                    'service_ticket': '3:serv:aBcDeF',
                }]
            return_raw: If set to True raw response will be returned instead of processed one

        Returns:
            List of token revocation statuses

        Raises:
            ClientError: An error occurred doing request
        """
        url = self.host + '/1/tokens/revoke/'
        request_data = noneless_dict({
            'tokenized_requests': tokenized_requests,
        })

        params = dict()
        if consumer is not None:
            params['consumer'] = consumer
        else:
            logger.warning(
                'send_tokenized_revoke_requests: The consumer must pass the consumer parameter containing A. B to the handle, '
                'where A is the consumer\'s name, which is agreed together with the quota '
                'for the handle, B is the version of the binary that will go into the handle.'
            )

        r = self._call_native_client(
            'post', url, data=request_data,
            send_service_ticket_only=True,
            params=params,
        )
        if return_raw:
            return r  # pragma: no cover

        data = self.validate_response(r)
        return data['result']

    def create_complete_secret(self, name, comment=None, tags=None, secret_version=None, roles=None,
                               return_raw=False):
        """Create secret with version and roles in one shot.

        Args:
            name: Non unique secret name
            comment: Secret comment
            tags: comma-separated list of tags
            secret_version: first secret version dict(value, comment)
            roles: comma-separated list of roles
            return_raw: If set to True raw response will be returned instead of processed one

        Returns:
            Dict(uuid, secret_version, status) or raw response depending on return_raw parameter.

        Raises:
            ClientError: An error occurred doing request
        """
        url = self.host + '/web/secrets/'
        if secret_version:
            secret_version = noneless_dict({
                'comment': secret_version.get('comment'),
                'value': self.unpack_value(secret_version['value']),
            })
        request_data = noneless_dict({
            'name': name,
            'comment': comment,
            'tags': tags,
            'secret_version': secret_version,
            'roles': roles,
        })

        r = self._call_native_client('post', url, data=request_data)
        if return_raw:
            return r  # pragma: no cover

        data = self.validate_response(r)
        return data

    def list_bundles(self, order_by=None, asc=False, page=None, page_size=None, return_raw=False):
        """List of bundles (without value).

        Args:
            order_by: order field
            asc: True if ascending order required, False otherwise
            page: Page number
            page_size: page size
            return_raw: If set to True raw response will be returned instead of processed one

        Returns:
            A list of secrets or raw response depending on return_raw parameter.

        Raises:
            ClientError: An error occurred doing request
        """
        url = self.host + '/1/bundles/'
        request_data = noneless_dict({
            'order_by': order_by,
            'asc': asc,
            'page': page,
            'page_size': page_size,
        })

        r = self._call_native_client('get', url, data=request_data)
        if return_raw:
            return r  # pragma: no cover

        data = self.validate_response(r)
        return data['bundles']

    def create_bundle(self, name, comment=None, return_raw=False):
        """Create new bundle.

        Create new bundle authorizing user by TVM2.0 user ticker or SSH private keys.
        SSH validation conducted through signature validation by public keys retrieved
        from https://staff.yandex-team.ru/

        Args:
            name: Non unique secret name
            comment: Secret comment
            return_raw: If set to True raw response will be returned instead of processed one

        Returns:
            Secret UUID or raw response depending on return_raw parameter.

        Raises:
            ClientError: An error occurred doing request
        """
        url = self.host + '/1/bundles/'
        request_data = noneless_dict({'name': name, 'comment': comment})

        r = self._call_native_client('post', url, data=request_data)
        if return_raw:
            return r  # pragma: no cover

        data = self.validate_response(r)
        return data['uuid']

    def update_bundle(self, bundle_uuid, name=None, comment=None, state=None, return_raw=False):
        """Update bundle metadata.

        Args:
            bundle_uuid: Bundle UUID
            name: New bundle name
            comment: New bundle comment
            state: state, one of 'normal' or 'hidden'
            return_raw: If set to True raw response will be returned instead of processed one

        Returns:
            True if OK or raw response depending on return_raw parameter.

        Raises:
            ClientError: An error occurred doing request
        """
        url = self.host + '/1/bundles/%s/' % bundle_uuid
        request_data = noneless_dict({'name': name, 'comment': comment, 'state': state})

        r = self._call_native_client('post', url, data=request_data)
        if return_raw:
            return r  # pragma: no cover

        self.validate_response(r)
        return True

    def get_bundle(self, bundle_uuid, page=None, page_size=None, return_raw=False):
        """Get bundle meta information.

        Args:
            bundle_uuid: Bundle UUID
            page: Page number
            page_size: page size
            return_raw: If set to True raw response will be returned instead of processed one

        Returns:
            A dict with secret meta information or raw response depending on return_raw parameter.

        Raises:
            ClientError: An error occurred doing request
        """
        url = self.host + '/1/bundles/%s/' % bundle_uuid
        request_data = noneless_dict({'page': page, 'page_size': page_size})

        r = self._call_native_client('get', url, data=request_data)
        if return_raw:
            return r  # pragma: no cover

        data = self.validate_response(r)
        return data['bundle']

    def create_bundle_version(self, bundle_uuid, secret_versions, return_raw=False):
        """Create new bundle version.

        Args:
            bundle_uuid: Bundle UUID
            secret_versions: List of secret UUIDs
            return_raw: If set to True raw response will be returned instead of processed one

        Returns:
            Secret version UUID or raw response depending on return_raw parameter.

        Raises:
            ClientError: An error occurred doing request
        """
        url = self.host + '/1/bundles/%s/versions/' % bundle_uuid
        request_data = noneless_dict({
            'secret_versions': secret_versions,
        })

        r = self._call_native_client('post', url, data=request_data)

        if return_raw:
            return r  # pragma: no cover

        data = self.validate_response(r)
        return data['bundle_version']
