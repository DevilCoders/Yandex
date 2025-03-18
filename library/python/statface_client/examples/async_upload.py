#!/usr/bin/env python
# coding: utf-8
# pylint: disable=invalid-name

from statface_client import *
from statface_client.api import *

# в конфиге указываю только те опции, что мне интересны
client_config = {
    'reports': {'upload_data': {'check_action_in_case_error': JUST_ONE_CHECK}}
}
client = StatfaceClient(client_config=client_config)
report = client.get_report('Adhoc/Adhoc/veronikaiv/1')
big_data = [{
    'fielddate': '2015-12-01 00:00:00',
    'lalala': '1',
    'package': str(idx),
    'count': str(idx)
    } for idx in range(1000)]
report.upload_data(
    scale=MONTHLY_SCALE,
    data=big_data,
    request_kwargs=dict(timeout=5),  # параметр requests
    _request_timeout=10  # параметр Statface
)
