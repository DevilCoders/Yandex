import json
import os
import random
import requests
import time

from cloud.blockstore.pylibs.common import retry


class Z2Client:

    class Error(Exception):
        pass

    _Z2_URL = 'https://z2-cloud.yandex-team.ru/'

    def __init__(self, token, logger, force=False):
        self._token = token
        self._logger = logger
        self._force = '1' if force else '0'

    def edit(self, group, packages):
        self._logger.info('group: %s' % group)
        self._logger.info('packages: %s' % packages)

        payload = {
            'configId': group,
            'apiKey': self._token,
            'items': json.dumps(packages),
        }
        self._logger.debug('payload: %s' % payload)
        response = requests.post(
            os.path.join(self._Z2_URL, 'api/v1/editItems'),
            data=payload)
        response.raise_for_status()

    @retry(tries=60, delay=60)
    def update_sync(self, group):
        self.update_async(group)
        while True:
            status = self.status(group)
            if status['updateStatus'] != 'FINISHED':
                time.sleep(10 + random.randint(-5, 5))
                continue
            if status['result'] == 'SUCCESS':
                return
            raise Exception('failed to update workers: %s' % status['failedWorkers'])

    def update_async(self, group):
        payload = {
            'configId': group,
            'apiKey': self._token,
            'forceYes': self._force,
        }
        response = requests.post(
            os.path.join(self._Z2_URL, 'api/v1/update'),
            data=payload
        )
        response.raise_for_status()

    def status(self, group):
        payload = {
            'configId': group,
            'apiKey': self._token
        }
        response = requests.get(
            os.path.join(self._Z2_URL, 'api/v1/updateStatus'),
            params=payload
        )
        response.raise_for_status()
        return response.json()['response']

    def packages(self, group):
        payload = {
            'configId': group,
            'apiKey': self._token
        }
        response = requests.get(
            os.path.join(self._Z2_URL, 'api/v1/items'),
            params=payload
        )
        response.raise_for_status()

        if not response.json()['success']:
            raise Exception('failed to fetch packages: %s' % response.json())

        return response.json()['response']['items']
