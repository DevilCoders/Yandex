#!/usr/bin/env python
# coding: utf-8
# pylint: disable=invalid-name

from __future__ import division, absolute_import, print_function, unicode_literals

import os
import logging
from statface_client import StatfaceClient, STATFACE_PRODUCTION

logger = logging.getLogger('statface_client_examples')

if os.environ.get('SFCLIEX_LOGGING_CONVENIENCE'):
    from pyaux.runlib import init_logging
    init_logging(level=logging.DEBUG)
    logging.root.handlers[0].formatter._fmt += '\n'
else:
    logging.basicConfig(level=logging.DEBUG)


path = 'Adhoc/Adhoc/ExampleReport'
title = 'Отчёт.'
config_in_yaml = """
---
dimensions:
- fielddate: date
- project: string
measures:
- value: number
title: Отчёт.
"""
data = (
    dict(fielddate='2018-01-02', project='upgen01', value=2),
    dict(fielddate='2018-01-02', project='upgen02', value=3),
)

client = StatfaceClient(host=STATFACE_PRODUCTION)
report = client.get_report(path)
report.upload_data(scale='daily', data=(item for item in data))
report.upload_data(scale='daily', data=(item for item in data), format='json')

# # Not supported anymore:
# data_lines = (
#     'fielddate=2018-01-02\tproject=upgen03\tvalue=4',
#     'fielddate=2018-01-02\tproject=upgen04\tvalue=5',
# )
# report.upload_data(scale='daily', data=(line for line in data_lines), format='tskv')
