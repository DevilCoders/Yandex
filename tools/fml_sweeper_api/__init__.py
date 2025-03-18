import requests
import logging

from .exceptions import SweeperError

logging.basicConfig(format='%(asctime)s - %(name)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)


class BaseApi(object):
    def __init__(self, oauth_token, server, api_prefix, is_json):
        self._server = server
        self._api_prefix = api_prefix
        self._is_json = is_json

        self._session = requests.Session()
        self._session.headers.update({
            'Authorization': 'OAuth {}'.format(oauth_token),
            'Content-Type': 'application/json',
            'Accept': 'application/json' if is_json else 'text/plain',
        })

    @property
    def url(self):
        return 'https://' + self._server + '/' + self._api_prefix

    def do_request(self, params):
        """
        Make request to API, return parsed result

        :param params: dict
        :return: dict
        """
        request_args = {
            'timeout': 300,
            'verify': True,
        }
        if self._is_json:
            request_args['json'] = params
        else:
            request_args['params'] = params

        r = self._session.post(self.url, **request_args)
        try:
            r.raise_for_status()
        except requests.exceptions.RequestException:
            raise SweeperError("Request to FML API failed:\n{}".format(r.text))
        else:
            return r.json()


class SweeperApi(BaseApi):
    def __init__(self, oauth_token):
        super(SweeperApi, self).__init__(oauth_token, 'fml.yandex-team.ru', 'rest/api/sweeper/task/run', True)


class FMLViewApi(BaseApi):
    def __init__(self, oauth_token):
        super(FMLViewApi, self).__init__(oauth_token, 'fml.yandex-team.ru', 'view/progress', False)


class FMLDownloadApi(BaseApi):
    def __init__(self, oauth_token):
        super(FMLDownloadApi, self).__init__(oauth_token, 'fml.yandex-team.ru', 'download/sweeper/task/result', False)
