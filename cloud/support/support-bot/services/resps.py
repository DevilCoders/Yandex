#!/usr/bin/env python3
"""This module contains RespsClient class."""

from core.error import RespsError
from core.objects.base import Base
from core.objects.duty import YandexCloudDuty

from utils.request import Request
from utils.config import Config


class RespsClient(Base):
    """This class provide interface for work with GoRe."""

    def __init__(self,
                 token=None,
                 endpoint=None,
                 request=None,
                 timeout=None,
                 **kwargs):

        self.token = token # not required for public methods
        self.endpoint = endpoint or Config.RESPS_ENDPOINT
        self.timeout = timeout

        self._other = kwargs

        if request:
            self._request = request
            self._request.set_and_return_client(self)
        else:
            self._request = Request(self, timeout=self.timeout)

    def get_service_duty(self, service='support', since=None, until=None):
        """Return oncall list. Since, until format: 01-01-1970T00:00."""
        url = f'{self.endpoint}/api/v0/duty/{service}'

        if since:
            url += f'?from={since}'
        if until:
            url += f'&to={until}'

        try:
            response = self._request.get(url, verify=False)
            return YandexCloudDuty.de_list(response, self)
        except Exception as error:
            raise RespsError(error)