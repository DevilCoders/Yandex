# coding: utf-8

import base64
import hashlib
import re

import paramiko
from paramiko.ssh_exception import (
    PasswordRequiredException,
    SSHException,
)
from six import StringIO

from .errors import (
    ClientInvalidRsaKeyNumber,
    ClientInvalidRsaPrivateKey,
    ClientNoKeysInSSHAgent,
    ClientRsaKeyHashNotFound,
    ClientRsaKeyRequiredPassword,
    ClientSSHAgentError,
    ClientUnknownKeyHashType,
)


class BaseRSAAuth(object):
    def __init__(self):
        self.keys = []

    def __call__(self):
        return self.keys


class RSAPrivateKeyAuth(BaseRSAAuth):
    def __init__(self, private_key, login=None):
        super(RSAPrivateKeyAuth, self).__init__()
        self.private_key = private_key
        try:
            self.keys = [
                paramiko.RSAKey.from_private_key(
                    StringIO(self.private_key),
                ),
            ]
        except PasswordRequiredException:
            raise ClientRsaKeyRequiredPassword()
        except SSHException:
            raise ClientInvalidRsaPrivateKey()


class BaseSSHAgentAuth(BaseRSAAuth):
    def __init__(self, ssh_agent=None):
        self._ssh_agent = ssh_agent or paramiko.agent.Agent()

    def fetch_keys(self):
        try:
            rsa_keys = self._ssh_agent.get_keys()
        except SSHException as e:
            raise ClientSSHAgentError(message=str(e))

        if not rsa_keys:
            raise ClientNoKeysInSSHAgent()
        return rsa_keys


class RSASSHAgentAuth(BaseSSHAgentAuth):
    def __init__(self, key_num=None, ssh_agent=None):
        super(RSASSHAgentAuth, self).__init__(ssh_agent=ssh_agent)
        self.key_num = key_num if type(key_num) is int else None

    def __call__(self):
        rsa_keys = self.fetch_keys()

        if self.key_num is not None:
            if self.key_num < 0 or self.key_num >= len(rsa_keys):
                raise ClientInvalidRsaKeyNumber(key_num=self.key_num)
            rsa_keys = rsa_keys[self.key_num:self.key_num + 1]
        return rsa_keys


class RSASSHAgentHash(BaseSSHAgentAuth):
    def __init__(self, key_hash, ssh_agent=None):
        super(RSASSHAgentHash, self).__init__(ssh_agent=ssh_agent)
        self._original_key_hash = key_hash
        self._hash_types = {
            'sha256': {
                're': re.compile(r'^(?:SHA256:)?([0-9a-zA-Z+/]{42,44})$'),
                'func': self.hash_key_sha256,
            },
            'md5': {
                're': re.compile(r'^(?:MD5:)?((?:[0-9a-f]{2}:){15}[0-9a-f]{2})$'),
                'func': self.hash_key_md5,
            },
            'sha1': {
                're': re.compile(r'^(?:SHA1:)?([0-9a-zA-Z+/]{27,29})$'),
                'func': self.hash_key_sha1,
            },
        }
        self.key_hash, self._hash_func = self.detect_hash_function(key_hash.strip() or '')

    def detect_hash_function(self, key_hash):
        for ht in self._hash_types.values():
            match = ht['re'].match(key_hash)
            if match:
                return match.group(1), ht['func']
        raise ClientUnknownKeyHashType(key_hash=key_hash)

    def hash_key_sha256(self, key):
        return base64.b64encode(hashlib.sha256(key.asbytes()).digest()).rstrip('=')

    def hash_key_md5(self, key):
        return ':'.join('{:02x}'.format(ord(c)) for c in key.get_fingerprint())

    def hash_key_sha1(self, key):
        return base64.b64encode(hashlib.sha1(key.asbytes()).digest()).rstrip('=')

    def __call__(self):
        rsa_keys = self.fetch_keys()

        for key in rsa_keys:
            if self.key_hash == self._hash_func(key):
                return [key]
        raise ClientRsaKeyHashNotFound(key_hash=self._original_key_hash)
