# coding: utf8

from __future__ import division, absolute_import, print_function, unicode_literals

import json

import pytest

from statface_client.api import StatfaceReportAPI
from statface_client.constants import \
    (JSON_DATA_FORMAT, CSV_DATA_FORMAT)
from statface_client.errors import StatfaceClientValueError


def test_serialize_data(report_example_tskv):
    data = report_example_tskv
    assert data == StatfaceReportAPI._serialize_data(data)

    data = {'one': 'two', 'three': 'four'}
    serialized_data = StatfaceReportAPI._serialize_data(
        data,
        data_format=JSON_DATA_FORMAT
    )
    assert json.loads(serialized_data)['values'] == [data]

    listed_data = [{1: 2}, {3: 4}]
    StatfaceReportAPI._serialize_data(listed_data, data_format=JSON_DATA_FORMAT)

    with pytest.raises(NotImplementedError):
        StatfaceReportAPI._serialize_data({1: 2}, data_format=CSV_DATA_FORMAT)

    class Foo(object):
        pass

    with pytest.raises(StatfaceClientValueError):
        StatfaceReportAPI._serialize_data(Foo())
