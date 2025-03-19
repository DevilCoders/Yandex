"""
Client API for mocks implemented using server_mock package.
"""

import requests

from tests.helpers import docker


class MockClient:
    """
    Client for mocks implemented using server_mock package.
    """

    def __init__(self, context, mock_name):
        self.context = context
        self.mock_name = mock_name

        host, port = docker.get_exposed_port(
            docker.get_container(context, mock_name + '01'), context.conf['projects'][mock_name]['expose']['http'])
        self.url_base = 'http://{0}:{1}'.format(host, port)

    def get_invocations(self):
        """
        Return invocations made on the mock.
        """
        response = requests.get(self.url_base + '/mock/invocations')
        response.raise_for_status()

        return response.json()

    def set_rule(self, path, status_code, json=None, times=None):
        """
        Set mock rule that defines behavior for requests to given URL path.

        The parameters 'status_code' and 'json' set response content returned
        on requests.

        If passed, 'times' determines the number of requests for which the rule
        will be active. After that, the previous behavior will be restored.
        """
        response = requests.post(
            self.url_base + '/mock/rules',
            json={
                'path': path,
                'status_code': status_code,
                'json': json,
                'times': times,
            })
        response.raise_for_status()

    def reset(self):
        """
        Reset mock to the initial state.
        """
        response = requests.post(self.url_base + '/mock/reset')
        response.raise_for_status()

    def ping(self):
        """
        Check mock availability
        """
        response = requests.get(self.url_base + '/ping')
        response.raise_for_status()
