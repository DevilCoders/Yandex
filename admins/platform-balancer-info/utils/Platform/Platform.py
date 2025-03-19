import logging
import os
import sys

import requests
from retry.api import retry_call

from .Schemas import Environment, Project, Instance, L7Balancer
from .Exceptions import InvalidPlatformToken

PLATFORM_TOKEN = "PLATFORM_TOKEN"
ENDPOINT = "https://platform.yandex-team.ru"


def platform_token():
    token = os.getenv(PLATFORM_TOKEN)
    if token:
        return token

    raise InvalidPlatformToken(f"Can not find {PLATFORM_TOKEN} in the environment")


class Platform:
    def __init__(self, project: str, token: str = None, log: logging.Logger = None):
        self._token = token
        self.log = log if log else logging
        self._project = project

    @property
    def token(self):
        if not self._token:
            try:
                self._token = platform_token()
            except InvalidPlatformToken as exc:
                self.log.critical(exc)
                sys.exit()
        return self._token

    def project(self) -> Project:
        path = f"/api/v1/project/{self._project}"
        return Project.Schema().load(self._get(path).json())

    def balancer(self, balancer) -> L7Balancer:
        path = f"/api/v1/balancerL7/{balancer}"
        return L7Balancer.Schema().load(self._get(path).json())

    def instance(self, obj_id, version) -> Instance:
        path = f"/api/v1/runtime/{obj_id}/{version}"
        return Instance.Schema().load(self._get(path).json())

    def environment(self, env_id) -> Environment:
        path = f"/api/v1/environment/stable/{env_id}"
        return Environment.Schema().load(self._get(path).json())

    def _get(self, path):
        return retry_call(self._make_req, fargs=["GET", path], tries=3, delay=2, backoff=2,
                          exceptions=requests.exceptions.ConnectionError)

    def _make_req(self, method, path):
        headers = {
            "Authorization": f"OAuth {self.token}"
        }

        try:
            req = requests.request(method, ENDPOINT + path, headers=headers)
            req.raise_for_status()
        except requests.HTTPError as exc:
            self.log.critical(exc)
        except requests.exceptions.ConnectionError as exc:
            raise

        return req
