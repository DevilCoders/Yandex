#!/usr/bin/env python3
"""This module contains QuotaCalc class."""

from app.quota.base import Base
from app.quota.constants import SERVICES


class QuotaCalculator(Base):
    """This class provides an interface for Quota calculator."""

    def __init__(self,
                 endpoint=None,
                 client=None,
                 request=None,
                 **kwargs):

        self.name = 'quota-calculator'
        self.endpoint = endpoint

        self._client = client
        self._request = request or SERVICES[self.name]['client']
