from datetime import datetime
from types import GeneratorType

import pytest
import pytz

from cloud.dwh.utils.datetimeutils import dttm_paginator
from cloud.dwh.utils.datetimeutils import parse_isoformat_to_tz_dttm
from cloud.dwh.utils.datetimeutils import parse_isoformat_to_msk_dttm
from cloud.dwh.utils.datetimeutils import MSK_TIMEZONE
from cloud.dwh.utils.datetimeutils import MSK_TIMEZONE_OBJ


@pytest.mark.parametrize('from_dttm, to_dttm, page_size, expected', [
    (datetime(2020, 12, 31, 0, 0, 1), datetime(2020, 12, 31, 0, 0, 0), 10, []),
    (datetime(2020, 12, 31, 0, 0, 0), datetime(2020, 12, 31, 0, 0, 0), 10, [(datetime(2020, 12, 31, 0, 0, 0), datetime(2020, 12, 31, 0, 0, 0))]),
    (datetime(2020, 12, 31, 0, 0, 0), datetime(2020, 12, 31, 0, 0, 9), 10, [(datetime(2020, 12, 31, 0, 0, 0), datetime(2020, 12, 31, 0, 0, 9))]),
    (datetime(2020, 12, 31, 0, 0, 0), datetime(2020, 12, 31, 0, 0, 10), 10, [(datetime(2020, 12, 31, 0, 0, 0), datetime(2020, 12, 31, 0, 0, 10))]),
    (
        datetime(2020, 12, 31, 0, 0, 0),
        datetime(2020, 12, 31, 0, 0, 30),
        10,
        [
            (datetime(2020, 12, 31, 0, 0, 0), datetime(2020, 12, 31, 0, 0, 10)),
            (datetime(2020, 12, 31, 0, 0, 11), datetime(2020, 12, 31, 0, 0, 21)),
            (datetime(2020, 12, 31, 0, 0, 22), datetime(2020, 12, 31, 0, 0, 30)),
        ]
    ),
    (
        datetime(2020, 12, 31, 0, 0, 0),
        datetime(2020, 12, 31, 0, 0, 2),
        0,
        [
            (datetime(2020, 12, 31, 0, 0, 0), datetime(2020, 12, 31, 0, 0, 0)),
            (datetime(2020, 12, 31, 0, 0, 1), datetime(2020, 12, 31, 0, 0, 1)),
            (datetime(2020, 12, 31, 0, 0, 2), datetime(2020, 12, 31, 0, 0, 2)),
        ]
    ),
])
def test_dttm_paginator(from_dttm, to_dttm, page_size, expected):
    result = dttm_paginator(from_dttm, to_dttm, page_size)

    assert isinstance(result, GeneratorType)
    assert list(result) == expected


@pytest.mark.parametrize('iso_string, tz, expected', [
    ('2021-02-03T04:05:06', pytz.UTC, datetime(2021, 2, 3, 4, 5, 6, tzinfo=pytz.UTC)),
    ('2021-02-03T04:05:06', pytz.timezone(MSK_TIMEZONE), MSK_TIMEZONE_OBJ.localize(datetime(2021, 2, 3, 4, 5, 6))),
])
def test_parse_isoformat_to_tz_dttm(iso_string, tz, expected):
    result = parse_isoformat_to_tz_dttm(iso_string, tz=tz)

    assert result == expected


def test_parse_isoformat_to_msk_dttm():
    result = parse_isoformat_to_msk_dttm('2021-02-03T04:05:06')

    assert result == MSK_TIMEZONE_OBJ.localize(datetime(2021, 2, 3, 4, 5, 6))
