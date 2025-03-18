# coding: utf-8
from __future__ import unicode_literals

import requests


class FakeResponse(requests.Response):

    def __init__(self, status_code=200, json=None):
        super(FakeResponse, self).__init__()
        self.status_code = status_code
        self.json_data = json or {}

    def json(self, **kwargs):
        return self.json_data
