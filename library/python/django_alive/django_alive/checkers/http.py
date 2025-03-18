# coding: utf-8
from __future__ import unicode_literals, absolute_import

import requests

from .base import Checker


class HTTPChecker(Checker):

    def __init__(self, name, stamper, url, headers=None, ok_statuses=None, timeout=None, method=None):
        super(HTTPChecker, self).__init__(name, stamper)

        self.url = url
        self.headers = headers or None
        self.ok_statuses = ok_statuses or [200]
        self.timeout = timeout or 1
        self.method = method or 'head'

    def check(self, group):
        try:
            response = requests.request(
                method=self.method,
                url=self.url,
                timeout=self.timeout,
                headers=self.headers,
                allow_redirects=False,
                verify=False,
            )
        except requests.RequestException as e:
            return {'error': str(e)}

        return {'status': response.status_code}

    def get_stamp_status(self, stamp):
        base_status = super(HTTPChecker, self).get_stamp_status(stamp)

        return stamp.data.get('status') in self.ok_statuses and base_status
