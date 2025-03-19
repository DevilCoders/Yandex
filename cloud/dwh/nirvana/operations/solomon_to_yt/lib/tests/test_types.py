from datetime import datetime

import pytest

from cloud.dwh.nirvana.operations.solomon_to_yt.lib.exceptions import InvalidYtSplitInterval
from cloud.dwh.nirvana.operations.solomon_to_yt.lib.types import YtSplitIntervals


def test_yt_split_intervals_check_interval_error():
    with pytest.raises(InvalidYtSplitInterval):
        YtSplitIntervals.check_interval('invalid')


def test_yt_split_intervals_check_interval():
    result = YtSplitIntervals.check_interval(YtSplitIntervals.Types.DAILY)
    assert result is None


@pytest.mark.parametrize('interval, expected', [
    (YtSplitIntervals.Types.MONTHLY, '1mo'),
    (YtSplitIntervals.Types.DAILY, '1d'),
    (YtSplitIntervals.Types.HOURLY, '1h'),
])
def test_yt_split_intervals_get_interval_short_name_error(interval, expected):
    with pytest.raises(KeyError):
        YtSplitIntervals.get_interval_short_name('invalid')


def test_yt_split_intervals_get_interval_short_name():
    result = YtSplitIntervals.check_interval(YtSplitIntervals.Types.DAILY)
    assert result is None


@pytest.mark.parametrize('interval, dttm, expected', [
    (YtSplitIntervals.Types.MONTHLY, datetime(2020, 1, 1, 0, 0, 0), datetime(2020, 1, 31, 23, 59, 59)),
    (YtSplitIntervals.Types.MONTHLY, datetime(2020, 1, 1, 2, 3, 4), datetime(2020, 1, 31, 23, 59, 59)),
    (YtSplitIntervals.Types.MONTHLY, datetime(2020, 1, 31, 23, 59, 59), datetime(2020, 1, 31, 23, 59, 59)),
    (YtSplitIntervals.Types.MONTHLY, datetime(2020, 2, 3, 4, 5, 6), datetime(2020, 2, 29, 23, 59, 59)),
    (YtSplitIntervals.Types.MONTHLY, datetime(2019, 2, 3, 4, 5, 6), datetime(2019, 2, 28, 23, 59, 59)),

    (YtSplitIntervals.Types.DAILY, datetime(2020, 1, 1, 0, 0, 0), datetime(2020, 1, 1, 23, 59, 59)),
    (YtSplitIntervals.Types.DAILY, datetime(2020, 2, 1, 2, 3, 4), datetime(2020, 2, 1, 23, 59, 59)),
    (YtSplitIntervals.Types.DAILY, datetime(2020, 3, 1, 23, 59, 59), datetime(2020, 3, 1, 23, 59, 59)),

    (YtSplitIntervals.Types.HOURLY, datetime(2020, 1, 1, 0, 0, 0), datetime(2020, 1, 1, 0, 59, 59)),
    (YtSplitIntervals.Types.HOURLY, datetime(2020, 2, 1, 2, 3, 4), datetime(2020, 2, 1, 2, 59, 59)),
    (YtSplitIntervals.Types.HOURLY, datetime(2020, 3, 1, 23, 59, 59), datetime(2020, 3, 1, 23, 59, 59)),
])
def test_yt_split_intervals_ceil_to_interval(interval, dttm, expected):
    assert YtSplitIntervals.ceil_to_interval(dttm, interval) == expected


@pytest.mark.parametrize('interval, dttm, expected', [
    (YtSplitIntervals.Types.MONTHLY, datetime(2020, 12, 31, 3, 4, 5), '2020-12-01'),
    (YtSplitIntervals.Types.DAILY, datetime(2020, 12, 31, 3, 4, 5), '2020-12-31'),
    (YtSplitIntervals.Types.HOURLY, datetime(2020, 12, 31, 3, 4, 5), '2020-12-31T03:00:00'),
])
def test_yt_split_intervals_format_by_interval(interval, dttm, expected):
    assert YtSplitIntervals.format_by_interval(dttm, interval) == expected
