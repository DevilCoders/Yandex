#!/usr/bin/env python
# coding: utf-8
# pylint: disable=invalid-name

from __future__ import division, absolute_import, print_function, unicode_literals

import logging
import statface_client

logger = logging.getLogger('statface_client')
logger.addHandler(logging.StreamHandler())
logger.setLevel(logging.INFO)


client = statface_client.ProductionStatfaceClient()

dict_data = '''
---
a: b  # comments should remain too
c: d
'''

dict_obj = client.get_stat_dict('test/sfcli_example')
dict_obj.upload(
    dict_data,
    language='ru',
    in_format='yaml',
    description='A dictionary from python-statface-client dict_upload example',
    allow_all_edit=False,
)
