#!/usr/bin/env python3
"""This module contains LoadBalancer class."""

from app.quota.base import Base
from app.quota.constants import SERVICES
from app.quota.subject import Subject


class LoadBalancer(Base):
    """This class provides an methods for Load Balancer quota service."""

    __DEFAULT_LIMITS__ = {
        'loadbalancer.networkLoadBalancers.count': 2,
        'loadbalancer.targetGroups.count': 100
    }

    def __init__(self,
                 client=None,
                 request=None,
                 **kwargs):

        self.name = 'load-balancer'
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
