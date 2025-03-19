import json
import os
import requests
import time


class Z2Client(object):
    def __init__(self, endpoint, token):
        self.endpoint = endpoint
        self.token = token

    def edit(self, group, packages):
        payload = {
            'configId': group,
            'apiKey': self.token,
            'items': json.dumps(packages)
        }

        response = requests.post(
            os.path.join(self.endpoint, 'api/v1/editItems'),
            data=payload
        )
        response.raise_for_status()

    def update_sync(self, group, force):
        self.update_async(group, force)

        while True:
            status = self.status(group)

            if status['updateStatus'] != 'FINISHED':
                time.sleep(5)
                continue

            if status['result'] == 'SUCCESS':
                return

            raise Exception('Failed to update workers: %s' % status['failedWorkers'])

    def update_async(self, group, force):
        payload = {
            'configId': group,
            'apiKey': self.token,
            'forceYes': '1' if force else '0'
        }

        response = requests.post(
            os.path.join(self.endpoint, 'api/v1/update'),
            data=payload
        )
        response.raise_for_status()

    def status(self, group):
        payload = {
            'configId': group,
            'apiKey': self.token
        }

        response = requests.get(
            os.path.join(self.endpoint, 'api/v1/updateStatus'),
            params=payload
        )
        response.raise_for_status()
        return response.json()['response']
