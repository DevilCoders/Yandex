#!/usr/bin/env python3
"""This module contains ResourceManager class."""

from quota.base import Base
from quota.constants import SERVICES
from quota.subject import Subject
from quota.error import FeatureNotImplemented


class ResourceManager(Base):
    """This class provides an methods for resource manager quota service."""

    __DEFAULT_LIMITS__ = {
        'active-operations-count': 15
    }

    def __init__(self,
                 client=None,
                 request=None,
                 **kwargs):

        self.name = 'resource-manager'
        self.client = client
        self._request = request or SERVICES[self.name]['client']
        self.endpoint = client.endpoint

    def get_metrics(self, subject: str):
        url = f'{self.endpoint}/compute/external/v1/folderQuota/{subject}'
        response = self._request(self.client).get(url)
        return Subject.de_json(response, self)

    def update_metric(self, subject: str, metric: str, value: int):
        url = f'{self.endpoint}/compute/external/v1/folderQuota/{subject}/metric'
        data = {
            "metric": {
                "name": metric,
                "limit": value
            }
        }
        return self._request(client=self.client).patch(url, json=data)

    def zeroize(self, subject: str):
        raise FeatureNotImplemented(f'Zeroize quota for {self.name} not supported')

    def set_to_default(self, subject: str):
        raise FeatureNotImplemented(f'Default quota for {self.name} not supported')
