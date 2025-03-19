"""
PostgreSQL host modify tests
"""

from test.mocks import checked_run_task_with_mocks
from test.tasks.postgresql.utils import get_postgresql_porto_host
from test.tasks.utils import check_alerts_synchronised


def test_postgresql_alert_group_create_alert_sync(mocker):
    """
    Check host modify mlock usage
    """
    args = {
        'hosts': {
            'host1': get_postgresql_porto_host(geo='geo1'),
            'host2': get_postgresql_porto_host(geo='geo2'),
            'host3': get_postgresql_porto_host(geo='geo3'),
        },
    }
    *_, state = checked_run_task_with_mocks(
        mocker, 'postgresql_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )

    check_alerts_synchronised(
        mocker,
        'postgresql_alert_group_create',
        {},
        state,
    )
