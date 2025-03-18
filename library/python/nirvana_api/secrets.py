import json
import requests
from os import path
from requests.packages.urllib3 import Retry


class SecretsApi(object):
    _grant_url = 'grants'
    _secret_url = 'secret/nirvana'

    def __init__(self, oauth_token, server='secrets.nirvana.yandex-team.ru', api_prefix='api/v1', max_retries=10, ssl_verify=True):
        self.server = server
        self.api_prefix = api_prefix
        self.oauth_token = oauth_token
        self.ssl_verify = ssl_verify
        self._session = requests.Session()
        retries = Retry(total=max_retries, backoff_factor=1, status_forcelist=[500, 502, 503, 504])
        self._session.mount('https://', requests.adapters.HTTPAdapter(max_retries=retries))

    def _get_url(self, relative_url=None):
        return path.join('https://', self.server, self.api_prefix, relative_url or '')

    def _get_headers(self):
        return {'Authorization': 'OAuth {0}'.format(self.oauth_token), 'Content-Type': 'application/json;charset=utf-8'}

    def create(self, name, description, secret_type, content, target):
        data = dict(name=name, description=description, type=secret_type, content=content, target=target)
        r = self._session.put(
            self._get_url(self._secret_url),
            headers=self._get_headers(),
            data=json.dumps(data),
            timeout=120,
            verify=self.ssl_verify,
        )
        return r.json() if r.status_code == 200 else r.text

    def get(self, name=None, get_content=False):
        relative_url = path.join(self._secret_url, name or '')
        if name and get_content:
            relative_url = path.join(relative_url, 'content')

        r = self._session.get(
            self._get_url(relative_url),
            headers=self._get_headers(),
            timeout=120,
            verify=self.ssl_verify,
        )
        return r.json() if r.status_code == 200 else r.text

    def list(self):
        r = self._session.get(
            self._get_url(self._secret_url),
            headers=self._get_headers(),
            timeout=120,
            verify=self.ssl_verify,
        )
        r.raise_for_status()
        return r.json()

    def update(self, name, description, secret_type, content, target):
        data = dict(name=name, description=description, type=secret_type, content=content, target=target)
        r = self._session.post(
            self._get_url(self._secret_url),
            headers=self._get_headers(),
            data=json.dumps(data),
            timeout=120,
            verify=self.ssl_verify,
        )
        return r.json() if r.status_code == 200 else r.text

    def delete(self, name):
        relative_url = path.join(self._secret_url, name)
        r = self._session.delete(
            self._get_url(relative_url),
            headers=self._get_headers(),
            timeout=120,
            verify=self.ssl_verify,
        )
        return r.text

    def get_grant(self, object_id):
        relative_url = path.join(self._grant_url, object_id)
        r = self._session.get(
            self._get_url(relative_url),
            headers=self._get_headers(),
            timeout=120,
            verify=self.ssl_verify,
        )
        return r.json() if r.status_code == 200 else r.text

    def add_grant(self, object_id, user, admin):
        data = dict(grantedObjectId=object_id, user=user, admin=admin)
        r = self._session.post(
            self._get_url(self._grant_url),
            headers=self._get_headers(),
            data=json.dumps(data),
            timeout=120,
            verify=self.ssl_verify,
        )
        return r.text

    def delete_grant(self, object_id, user, admin):
        data = dict(grantedObjectId=object_id, user=user, admin=admin)
        relative_url = path.join(self._grant_url, object_id)
        r = self._session.delete(
            self._get_url(relative_url),
            headers=self._get_headers(),
            data=json.dumps(data),
            timeout=120,
            verify=self.ssl_verify,
        )
        return r.text


class SecretType(object):
    token = 'TOKEN'
    private_key = 'PRIVATE_KEY'
    certificate = 'CERTIFICATE'
