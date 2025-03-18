import requests

from tools.releaser.src.cli import utils


# https://wiki.yandex-team.ru/runtime-cloud/nanny/service-repo-api/


class NannyClient:
    HOST = 'nanny.yandex-team.ru'

    def __init__(self, service):
        self.session = requests.Session()
        self.session.headers.update({
            'Authorization': f'OAuth {utils.get_oauth_token_or_panic()}'
        })
        self.service = service

    def make_request(self, method, path, data=None):
        url = f'https://nanny.yandex-team.ru/v2/services/{self.service}/{path}/'
        response = self.session.request(method, url, json=data)
        response.raise_for_status()
        return response

    def get_runtime_attrs(self):
        return self.make_request('GET', 'runtime_attrs').json()

    def update_runtime_attrs(self, data):
        return self.make_request('PUT', 'runtime_attrs', {
            'content': data['content'],
            'snapshot_id': data['_id'],
            'comment': 'update push-client config',
        }).json()
