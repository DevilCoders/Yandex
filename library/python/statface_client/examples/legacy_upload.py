#!/usr/bin/env python
# coding: utf-8
# pylint: disable=invalid-name

from __future__ import division, absolute_import, print_function, unicode_literals

import os
import logging
import tempfile

from statface_client import StatfaceClient
from statface_client.api import StatfaceReportAPI

logger = logging.getLogger('statface_client_examples')

if os.environ.get('SFCLIEX_LOGGING_CONVENIENCE'):
    from pyaux.runlib import init_logging
    init_logging(level=logging.DEBUG)
    logging.root.handlers[0].formatter._fmt += '\n'
else:
    logging.basicConfig(level=logging.DEBUG)


path = 'Adhoc/Adhoc/ExampleReport'
title = u'Отчёт.'
config_in_yaml = u"""
---
dimensions:
- fielddate: date
- project: string
measures:
- value: number
title: Отчёт.
""".encode('utf-8')
data = (  # NOTE: a single bytes() object, not an iterable.
    b'tskv\tfielddate=2018-01-02\t'
    b'project=prj\xa8s\xe3\xe0\xb5\x94]_\xd7{\xb6ZP\xfb(]x\tvalue=2\n'
    b'tskv\tfielddate=2018-01-02\t'
    b'project=prj\xe4\x85\xa9(55\xbd\xfb\x0e\xda\xd7\xc4\xa2\xc4\xd0\x9cx\tvalue=3\n'
)

client = StatfaceClient()
report = StatfaceReportAPI(client, path)
# report = client.get_report(path)

common_kwargs = dict(scale='daily', format='tskv')

logger.info('config_and_data')
report.upload_config_and_data(config=config_in_yaml, title=title, data=data, **common_kwargs)

logger.info('data')
report.upload_data(data=data, **common_kwargs)

_, filepath = tempfile.mkstemp(prefix='tmp_sfclient_')
try:
    with open(filepath, 'wb') as fo:
        fo.write(data)

    with open(filepath, 'rb') as fo:
        logger.info('config_and_data file-obj')
        report.upload_config_and_data(config=config_in_yaml, title=title, data=fo, **common_kwargs)

    with open(filepath, 'rb') as fo:
        logger.info('data file-obj')
        report.upload_data(data=fo, **common_kwargs)

finally:
    os.unlink(filepath)
