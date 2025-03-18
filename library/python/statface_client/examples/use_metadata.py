#!/usr/bin/env python
# coding: utf-8
# pylint: disable=invalid-name

from __future__ import division, absolute_import, print_function, unicode_literals

import json

from statface_client import StatfaceClient

client = StatfaceClient()
report = client.get_report('Adhoc/Adhoc/veronikaiv/2')
metadata = report._api.download_metadata()
print(metadata)
print(report.merge_metadata(
    data={'qqqq': json.dumps({1: 2}), 'responsible': ['2', '1']}
))
print(report.download_metadata())
print(report.merge_metadata(
    responsible=['thenno', 'hans'],
    extra_data=['1', 'w']
))
