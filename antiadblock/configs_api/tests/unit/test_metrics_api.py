from time import mktime
import datetime

import pytest

from antiadblock.configs_api.lib.metrics import helpers


@pytest.mark.parametrize('part, whole, expected_percent',
                         [(5, 100, 5),
                          ('5', '100', 5),
                          (5.0, 100.0, 5),
                          ('5.0', '100.0', 5),
                          (0, 100, 0),
                          (100, 0, 110),
                          (105, 100, 105)])
def test_percentage(part, whole, expected_percent):

    assert helpers.percentage(part, whole) == expected_percent


@pytest.mark.parametrize('date_ranges, expected_group_by',
                         [([1, 15, 60], '1m'),
                          ([61, 356, 720], '5m'),
                          ([721, 900, 1080], '10m'),
                          ([1081, 1250, 1440], '30m'),
                          ([1441, 1650, 99999], '60m')])
def test_date_params_from_range(date_ranges, expected_group_by):

    for date_range in date_ranges:
        from_date_ts, group_by = helpers.date_params_from_range(date_range)
        assert from_date_ts == int(mktime((datetime.datetime.now() - datetime.timedelta(minutes=date_range)).timetuple())) * 1000
        assert group_by == expected_group_by
