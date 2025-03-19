#!/usr/bin/env python3
"""This module contains YDB class."""

from quota.base import Base
from quota.constants import SERVICES
from quota.subject import Subject


class ALB(Base):
    """This class provides an methods for Application Load Balancer quota service."""

    __DEFAULT_LIMITS__ = {
        'apploadbalancer.loadBalancers.count': 4,
        'apploadbalancer.httpRouters.count': 16,
        'apploadbalancer.backendGroups.count': 32,
        'apploadbalancer.targetGroups.count': 32
            }

    def __init__(self,
                 client=None,
                 request=None,
                 **kwargs):

        self.name = 'application-load-balancer'
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
                metric.update(1)
                yield f'{metric.name} - OK (set to 1 cannot be set to 0)'
            except Exception as err:
                yield f'{metric.name} - FAIL: {err}'

    def set_to_default(self, subject: str):
        for metric, limit in self.__DEFAULT_LIMITS__.items():
            try:
                self.update_metric(subject, metric, limit)
                yield f'{metric} - OK'
            except Exception as err:
                yield f'{metric} - FAIL: {err}'
