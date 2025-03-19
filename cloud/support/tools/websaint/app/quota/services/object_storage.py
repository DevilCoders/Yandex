#!/usr/bin/env python3
"""This module contains ObjectStorage class."""

from app.quota.base import Base
from app.quota.constants import SERVICES
from app.quota.subject import Subject


class ObjectStorage(Base):
    """This class provides an methods for object storage quota service."""

    __DEFAULT_LIMITS__ = {
        'storage.volume.size': '5497558138880',
        'storage.buckets.count': '25'
    }

    def __init__(self,
                 client=None,
                 request=None,
                 **kwargs):

        self.name = 'object-storage'
        self.client = client
        self._request = request or SERVICES[self.name]['client']
        self.endpoint = client.endpoint

    def get_metrics(self, subject: str):
        url = f'{self.endpoint}/management/quota/cloud/{subject}'
        response = self._request(self.client).get(url)
        return Subject.de_json(response, self)

    def update_metric(self, subject: str, metric: str, value: int):
        url = f'{self.endpoint}/management/quota/cloud/{subject}'
        data = {
            "cloud_id": subject,
            "metrics": [
                {
                    "name": metric,
                    "limit": str(value)  # wtf? error: is not of type u'string'
                }
            ]
        }
        # FIXME: use PATCH method when it will be available
        return self._request(client=self.client).patch(url, json=data)

    def zeroize(self, subject: str):
        for metric in self.get_metrics(subject).metrics:
            try:
                value = 1 if metric.name == 'storage.volume.size' else 0
                metric.update(value)
                yield f'{metric.name} - OK'
            except Exception as err:
                yield f'{metric.name} - FAIL: {err}'

    def set_to_default(self, subject: str):
        for metric, limit in self.__DEFAULT_LIMITS__.items():
            try:
                self.update_metric(subject, metric, limit)
                yield f'{metric} - OK'
            except Exception as err:
                yield f'{metric} - FAIL: {err}'
