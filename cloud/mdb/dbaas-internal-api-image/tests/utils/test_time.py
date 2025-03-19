"""
tests for time utils
"""
from datetime import datetime, timedelta, timezone

import pytest

from dbaas_internal_api.utils.time import datetime_to_rfc3339_utcoffset, rfc3339_to_datetime

# pylint: disable=invalid-name,missing-docstring


class Test_rfc3339_to_datetime:
    @pytest.mark.parametrize(
        ('value', 'dt'),
        [
            ['1994-03-14T17:00:00Z', datetime(1994, 3, 14, 17, 0, 0, tzinfo=timezone.utc)],
            ['1994-03-14T17:00:00.0000Z', datetime(1994, 3, 14, 17, 0, 0, tzinfo=timezone.utc)],
            ['1994-03-14T17:00:00.1Z', datetime(1994, 3, 14, 17, microsecond=100000, tzinfo=timezone.utc)],
            ['1994-03-14T17:00:00+03:00', datetime(1994, 3, 14, 17, tzinfo=timezone(timedelta(hours=3)))],
        ],
    )
    def test_parse_valid(self, value, dt):
        assert rfc3339_to_datetime(value) == dt

    def test_dt_without_tz_is_invalid(self):
        with pytest.raises(ValueError):
            assert rfc3339_to_datetime('1994-03-14T17:00:00')

    @pytest.mark.parametrize(
        'value',
        [
            '',
            'today',
            '1s',
            '1994-03-14',
        ],
    )
    def test_strage_dt(self, value):
        with pytest.raises(ValueError):
            assert rfc3339_to_datetime(value)


class Test_datetime_to_rfc3339_utcoffset:
    @pytest.mark.parametrize(
        ('value', 'dt_str'),
        [
            [datetime(1994, 3, 14, 17, 0, 0, tzinfo=timezone.utc), '1994-03-14T17:00:00Z'],
            [datetime(1994, 3, 14, 17, microsecond=100000, tzinfo=timezone.utc), '1994-03-14T17:00:00.1Z'],
            [datetime(1994, 3, 14, 17, microsecond=3000, tzinfo=timezone.utc), '1994-03-14T17:00:00.003Z'],
            [datetime(1994, 3, 14, 17, microsecond=7, tzinfo=timezone.utc), '1994-03-14T17:00:00.000007Z'],
            [
                datetime(1994, 3, 14, 17, tzinfo=timezone(timedelta(hours=3))),
                '1994-03-14T14:00:00Z',
            ],
        ],
    )
    def test_good(self, value, dt_str):
        assert datetime_to_rfc3339_utcoffset(value) == dt_str

    def test_raise_for_naive_datetime(self):
        with pytest.raises(RuntimeError):
            datetime_to_rfc3339_utcoffset(datetime(1994, 3, 14, 17))
