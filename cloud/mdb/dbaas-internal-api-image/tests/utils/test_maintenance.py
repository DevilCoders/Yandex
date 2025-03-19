"""
Maintenance test
"""
import pytest
from typing import Optional

from datetime import datetime, timedelta, timezone

from dbaas_internal_api.utils.maintenance_helpers import calculate_nearest_maintenance_window
from dbaas_internal_api.utils.maintenance import reschedule_maintenance, MaxDelayExceeded
from dbaas_internal_api.utils.types import ClusterInfo, MaintenanceRescheduleType

"""
Mo  Tu  We  Th  Fr  Sa  Su
        1   2   3   4   5
6   7   8   9   10  11  12
13  14  15  16  17  18  19
20  21  22  23  24  25  26
27  28  29  30  31
"""


@pytest.mark.parametrize(
    'operation_time, window_day, window_hour, next_window_time',
    [
        [datetime(2020, 1, 1, 2), 'MON', 2, datetime(2020, 1, 6, 1)],
        [datetime(2020, 1, 6, 0), 'MON', 2, datetime(2020, 1, 6, 1)],
        [datetime(2020, 1, 6, 1), 'MON', 2, datetime(2020, 1, 13, 1)],
        [datetime(2020, 1, 6, 2), 'MON', 2, datetime(2020, 1, 13, 1)],
        [datetime(2020, 1, 2, 1), 'MON', 2, datetime(2020, 1, 6, 1)],
        [datetime(2020, 1, 5, 23), 'MON', 23, datetime(2020, 1, 6, 22)],
        [datetime(2020, 1, 6, 20), 'MON', 23, datetime(2020, 1, 6, 22)],
        [datetime(2020, 1, 7, 0), 'MON', 23, datetime(2020, 1, 13, 22)],
    ],
)
def test_all_response_annotations_is_absolute(operation_time, window_day, window_hour, next_window_time):
    assert calculate_nearest_maintenance_window(operation_time, window_day, window_hour) == next_window_time


def now(freezer):
    """
    Freeze time.
    """
    freezer.move_to('2017-05-20')
    return datetime.now(timezone.utc)


@pytest.mark.parametrize(
    "test_name,reschedule_type,raised_error,expected_time",
    [
        ("specific time exceeds max delay", MaintenanceRescheduleType.specific_time, MaxDelayExceeded, None),
        (
            "immediate replans on now",
            MaintenanceRescheduleType.immediate,
            None,
            datetime(2017, 5, 20, 0, 0, 0, 0, timezone.utc),
        ),
    ],
)
def test_reschedule_maintenance(
    mocker,
    freezer,
    test_name,
    reschedule_type: MaintenanceRescheduleType,
    raised_error: Optional[Exception],
    expected_time: Optional[datetime],
):
    assert test_name
    now_time = now(freezer)
    cluster = ClusterInfo.make(
        args_dict={
            'mw_max_delay': now_time,
            'cid': 'test_cid',
            'name': 'test_name',
            'type': 'test_type',
            'env': 'dev',
            'created_at': now_time,
            'network_id': 'test_network_id',
            'value': None,
            'description': 'test cluster description',
            'labels': None,
            'rev': 1,
            'backup_schedule': None,
            'host_group_ids': [],
            'status': 'RUNNING',
            'mw_info': 'some maintenance',
            'mw_config_id': 'mw_config_test_id',
            'mw_create_ts': now_time - timedelta(weeks=3),
            'mw_delayed_until': None,
            'mw_day': None,
            'mw_hour': None,
        }
    )
    mocker.patch("dbaas_internal_api.utils.maintenance.metadb.reschedule_maintenance_task", return_value=True)
    if raised_error is not None:
        with pytest.raises(raised_error):
            reschedule_maintenance(cluster, reschedule_type, now_time + timedelta(days=1))
    else:
        assert expected_time == reschedule_maintenance(cluster, MaintenanceRescheduleType.immediate, None)
