import requests
import os
import logging


class API:
    def __init__(self, token: str):
        pkg_dir, _ = os.path.split(__file__)
        self.pem_path = os.path.join(pkg_dir, 'yandex.pem')
        self.token = token
        self.log = logging.getLogger(self.__class__.__name__)

    def _fetch(self, url: str):
        """ Fetch data from API """
        result = requests.get(self.API + url,
                              timeout=10,
                              headers={'Authorization': 'OAuth %s' % self.token},
                              verify=self.pem_path)
        if result.status_code != 200:
            raise RuntimeError('Cannot fetch %s' % url)
        return result.json()
