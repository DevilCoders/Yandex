#!/usr/bin/env python
# -*- coding: utf-8 -*-

import re
import time
import uuid
import base64
import requests

VAULT_URL = 'http://nanny-vault.yandex-team.ru/v1'


class VaultClient:
    def __init__(self, oauth_token):
        self.session = None
        self.oauth_token = oauth_token
        self.make_vault_session()

    def make_vault_session(self):
        self.session = requests.Session()
        request = self.session.post('{}/auth/blackbox/login/keychain'.format(VAULT_URL),
                                    json={'oauth': self.oauth_token})
        request.raise_for_status()
        self.session.headers['X-Vault-Token'] = request.json()['auth']['client_token']

    def create_secret(self, keychain_id, secret_id, revision_name, content):
        if not re.match(r'^[a-zA-Z0-9-]+$', revision_name):
            raise ValueError('Revision id must contain only alphanumeric chars and "-"')
        full_rev_id = '{}&{}&{}'.format(uuid.uuid4(), revision_name, int(time.time()) * 1000)
        entries = []
        for key, value in content.iteritems():
            encoded = base64.b64encode(value)
            entries.append({
                'meta': {'format': 'BASE64'},
                'content': {
                    'key': key,
                    'value': encoded,
                }
            })
        self.session.put('{}/secret/{}/{}/{}'.format(VAULT_URL, keychain_id, secret_id, full_rev_id),
                         json={'content': {'entries': entries}}).raise_for_status()
