"""
MySQL alert group create tests
"""

from test.mocks import checked_run_task_with_mocks
from test.tasks.mysql.utils import get_mysql_porto_host
from test.tasks.utils import check_alerts_synchronised


def test_mysql_alert_group_create_alert_sync(mocker):
    """
    Check alert group create mlock usage
    """
    args = {
        'hosts': {
            'host1': get_mysql_porto_host(geo='geo1'),
            'host2': get_mysql_porto_host(geo='geo2'),
            'host3': get_mysql_porto_host(geo='geo3'),
        },
    }
    *_, state = checked_run_task_with_mocks(mocker, 'mysql_cluster_create', dict(**args, s3_bucket='test-s3-bucket'))

    check_alerts_synchronised(
        mocker,
        'mysql_alert_group_create',
        {},
        state,
    )
