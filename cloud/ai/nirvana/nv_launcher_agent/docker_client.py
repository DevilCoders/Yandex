import base64
import json
import requests_unixsocket
import logging

from cloud.ai.nirvana.nv_launcher_agent.lib.thread_logger import ThreadLogger


class DockerException(Exception):
    def __init__(self, message):
        self.message = message


class DockerClient:
    def __init__(self, repo_prefix, key_string):
        self.repo_prefix = repo_prefix
        self.key = {'username': "json_key", "password": json.dumps(key_string)}

    def _get_auth_header(self):
        return {
            "X-Registry-Auth": base64.b64encode(json.dumps(self.key).encode('ascii'))
        }

    def _get_base_url(self):
        return 'http+unix://%2Fvar%2Frun%2Fdocker.sock'

    def _post(self, url, params=None, data=None):
        session = requests_unixsocket.Session()
        method = self._get_base_url() + url

        params_query = ""
        if params is not None:
            params_query = '?'
            for key, value in params.items():
                params_query += f'{key}={value}&'
            params_query = params_query[:-1]

        ThreadLogger.info(f'Sending post request url={method}, params_query={params_query}')
        response = session.post(method + params_query, data=data, headers=self._get_auth_header())
        ThreadLogger.info(f'Response: {response}, status_code={response.status_code}')
        if response.status_code >= 400:
            raise DockerException(response.content)
        return response

    def push(self, image_name, tag='latest'):
        self._post(
            url=f'/images/{self.repo_prefix}/{image_name}/push',
            params={'tag': tag}
        )

    def tag(self, name, remote_name, tag='latest'):
        self._post(
            url=f'/images/{name}/tag',
            params={
                'repo': f'{self.repo_prefix}/{remote_name}',
                'tag': tag
            }
        )

    def pull(self, remote_image_name):
        self._post(
            url='/images/create',
            params={'fromImage': remote_image_name}
        )


def test():
    import os
    key = os.path.expanduser('~/cloud-nirvana-docker-key.json')
    with open(key, 'r') as f:
        key_string = json.load(f)

    docker = DockerClient(
        repo_prefix='cr.yandex/crppns4pq490jrka0sth/cloud-nirvana-images-cache',
        key_string=key_string
    )
    # docker.tag(
    #     name='a975f0d2-d872-4da3-bf67-2f7d99f71cf9',
    #     remote_name="shabby-aqua-hippopotamus-9ea",
    #     tag="latest"
    # )
    # docker.push(
    #     image_name='shabby-aqua-hippopotamus-9ea',
    #     tag="latest"
    # )
    docker.pull('cr.yandex/crppns4pq490jrka0sth/cloud-nirvana-images-cache/test-test-test-45ab')
