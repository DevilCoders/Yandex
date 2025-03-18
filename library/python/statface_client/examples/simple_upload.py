#!/usr/bin/env python
# coding: utf-8
# pylint: disable=invalid-name

from __future__ import division, absolute_import, print_function, unicode_literals

import os
import logging
import statface_client

logger = logging.getLogger('statface_client_examples')

if os.environ.get('SFCLIEX_LOGGING_CONVENIENCE'):
    from pyaux.runlib import init_logging
    init_logging(level=logging.DEBUG)
    logging.root.handlers[0].formatter._fmt += '\n'
else:
    logging.basicConfig(level=logging.DEBUG)


path = 'Adhoc/Adhoc/ExampleReport'
data = (
    dict(fielddate='2018-01-02', project='upgen01', value=2),
    dict(fielddate='2018-01-02', project='upgen02', value=3),
)

# sfcli = statface_client.ProductionStatfaceClient(_no_excess_calls=True)
sfcli = statface_client.BetaStatfaceClient(_no_excess_calls=True)
sfreport = sfcli.get_report(path)
sfreport.simple_upload(scale='daily', data=data)
