#!/usr/bin/env python
# -*- coding: utf-8 -*-
import struct
from base64 import urlsafe_b64encode
from time import time

import paramiko
import requests

OAUTH_API = 'https://oauth.yandex-team.ru/token'
OAUTH_CLIENT_ID = 'e10219f794ed414bbabf2075089f90fa'
OAUTH_CLIENT_SECRET = 'dd571d2a8d164aa8a8ccbccca9426874'  # Это не секретная информация


class OAuthError(Exception):
    pass


def extract_sign(signature_string):
    parts = []
    while signature_string:
        len = struct.unpack('>I', signature_string[:4])[0]
        bits = signature_string[4:len + 4]
        parts.append(bits)
        signature_string = signature_string[len + 4:]

    return parts[1]


def get_token(login):
    ts = int(time())
    string_to_sign = '%s%s%s' % (ts, OAUTH_CLIENT_ID, login)
    ssh_keys = paramiko.Agent().get_keys()
    if not ssh_keys:
        raise OAuthError('no ssh keys or ssh-agent not running')
    for private_key in ssh_keys:
        sign = extract_sign(private_key.sign_ssh_data(string_to_sign))
        resp = requests.post(
            OAUTH_API,
            data={
                'grant_type': 'ssh_key',
                'client_id': OAUTH_CLIENT_ID,
                'client_secret': OAUTH_CLIENT_SECRET,
                'login': login,
                'ts': ts,
                'ssh_sign': urlsafe_b64encode(sign),
            },
        )
        if resp.status_code < 500:
            rv = resp.json()
            if rv.get('error'):
                if rv['error'] == 'invalid_grant':
                    continue  # пробуем следующий ssh-ключ
                else:
                    raise OAuthError(rv.get('error_description', rv['error']))
            else:
                return rv['access_token']

    raise OAuthError('ssh_sign is not valid')
