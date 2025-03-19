#!/usr/bin/env python3
"""This module contains DBaaS class."""

from quota.base import Base
from quota.constants import SERVICES
from quota.subject import Subject


class DBaaS(Base):
    """This class provides an methods for DBaaS quota service."""

    __DEFAULT_LIMITS__ = {
        'mdb.hdd.size': 4398046511104,
        'mdb.ssd.size': 4398046511104,
        'mdb.memory.size': 549755813888,
        'mdb.gpu.count': 0,
        'mdb.cpu.count': 64,
        'mdb.clusters.count': 16
    }

    def __init__(self,
                 client=None,
                 request=None,
                 **kwargs):

        self.name = 'managed-database'
        self.client = client
        self._request = request or SERVICES[self.name]['client']
        self.endpoint = client.endpoint

    def get_metrics(self, subject: str):
        url = f'{self.endpoint}/mdb/v2/quota/{subject}'
        response = self._request(self.client).get(url)
        return Subject.de_json(response, self)

    def update_metric(self, subject: str, metric: str, value: int):
        url = f'{self.endpoint}/mdb/v2/quota'
        data = {
            "cloud_id": subject,
            "metrics": [
                {
                    "name": metric,
                    "limit": value
                }
            ]
        }
        return self._request(client=self.client).post(url, json=data)

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
