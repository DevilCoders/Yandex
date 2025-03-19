# -*- coding: utf-8 -*-
import base64
import copy
import os
import random
import re
import shlex
import string
import subprocess
from collections import OrderedDict

from retrying import retry
from library.python.vault_client.errors import ClientError
from library.python.vault_client.instances import Production

YAV_RE = re.compile(r'^(?P<uuid>(?:sec|ver)-[0-9a-z]{26,})(?:\[(?P<keys>.+?)\])?$', re.I)


@retry(wait_fixed=1000, stop_max_attempt_number=3)
def get_oauth_token(ya_command):
    cmd = f"{ya_command} vault oauth"
    result = subprocess.check_output(shlex.split(cmd), env=get_env_without_python_entry_point())
    return result.decode().strip()


def get_env_without_python_entry_point():
    env = copy.deepcopy(os.environ)
    if 'Y_PYTHON_ENTRY_POINT' in env:
        del env['Y_PYTHON_ENTRY_POINT']
    return env


def random_word(length=16):
    letters = string.ascii_lowercase
    return ''.join(random.choice(letters) for i in range(length))


class YandexVault(object):
    def __init__(self, oauth=None, ya_command=None):
        ya_command = ya_command or 'ya'
        if not oauth:
            oauth = get_oauth_token(ya_command)
        self.client = Production(authorization=f'Oauth {oauth}')
        self._cache = {}

    def _parse_key(self, key):
        matches = YAV_RE.match(key.strip())
        if not matches:
            return key
        secret_uuid = matches.group('uuid')
        keys = None
        if matches.group('keys'):
            keys = [s.strip() for s in matches.group('keys').split(',')]
        return secret_uuid, keys

    def _get_value(self, secret_uuid):
        if secret_uuid in self._cache:
            return self._cache[secret_uuid]
        try:
            value = self.client.get_version(secret_uuid, packed_value=False)['value']
        except ClientError:
            value = []
        packed_value = OrderedDict()
        for v in value:
            processed_value = v['value']
            encoding = v.get('encoding')
            if encoding and encoding == 'base64':
                processed_value = base64.b64decode(processed_value)
            packed_value[v['key']] = processed_value
        self._cache[secret_uuid] = packed_value
        return packed_value

    def _process_value(self, secret_uuid, value, keys=None):
        if not keys:
            return value

        result = {}
        for k in keys:
            if k not in value:
                value[k] = f'{secret_uuid}-{k}-{random_word()}'
            result[k] = value[k]

        if len(keys) == 1:
            return result.get(keys[0])
        return result

    def get(self, key):
        secret_uuid, keys = self._parse_key(key)
        value = self._get_value(secret_uuid)
        result = self._process_value(secret_uuid, value, keys)
        return result
