#!/usr/bin/env python3
"""This module contains Compute class."""

from quota.base import Base
from quota.constants import SERVICES
from quota.subject import Subject


class Compute(Base):
    """This class provides an methods for Compute quota service."""

    __DEFAULT_LIMITS__ = {
        'compute.ssdDisks.size': 214748364800,
        'compute.instanceCores.count': 32,
        'compute.snapshots.count': 32,
        'compute.images.count': 32,
        'compute.instanceGpus.count': 0,
        'compute.hddDisks.size': 536870912000,
        'compute.instances.count': 12,
        'compute.instanceMemory.size': 137438953472,
        'compute.placementGroups.count': 2,
        'compute.disks.count': 32,
        'compute.snapshots.size': 429496729600
    }

    def __init__(self,
                 client=None,
                 request=None,
                 **kwargs):

        self.name = 'compute'
        self.client = client
        self._request = request or SERVICES[self.name]['client']
        self.endpoint = client.endpoint

    def get_metrics(self, subject: str):
        response = self._request(self.client).get(self.name, self.endpoint, subject)
        return Subject.de_json(response, self)

    def update_metric(self, subject: str, metric: str, value: int):
        data = {
            "subject_id": subject,
            "name": metric,
            "limit": value
        }

        return self._request(self.client).update(self.name, self.endpoint, data)

    def zeroize(self, subject: str):
        for metric in self.get_metrics(subject).metrics:
            try:
                metric.update(0)
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


class ComputeOld(Base):
    """This class provides an methods for REST Compute quota service."""

    __DEFAULT_LIMITS__ = {}

    def __init__(self,
                 client=None,
                 request=None,
                 **kwargs):

        self.name = 'compute-old'
        self.client = client
        self._request = request or SERVICES[self.name]['client']
        self.endpoint = client.endpoint

    def get_metrics(self, subject: str):
        url = f'{self.endpoint}/compute/external/v1/quota/{subject}'
        response = self._request(self.client).get(url)
        return Subject.de_json(response, self)

    def update_metric(self, subject: str, metric: str, value: int):
        url = f'{self.endpoint}/compute/external/v1/quota/{subject}/metric'
        data = {
            "metric": {
                "name": metric,
                "limit": value
            }
        }
        return self._request(client=self.client).patch(url, json=data)

    def zeroize(self, subject: str):
        for metric in self.get_metrics(subject).metrics:
            try:
                metric.update(0)
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
