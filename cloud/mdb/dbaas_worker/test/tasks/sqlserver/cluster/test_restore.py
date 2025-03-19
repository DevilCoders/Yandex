"""
SQLServer cluster restore tests
"""

from test.mocks import get_state
from test.tasks.sqlserver.utils import get_sqlserver_compute_host
from test.tasks.utils import check_mlock_usage, check_task_interrupt_consistency


def test_compute_sqlserver_cluster_restore_interrupt_consistency(mocker):
    """
    Check compute restore interruptions
    """
    check_task_interrupt_consistency(
        mocker,
        'sqlserver_cluster_restore',
        {
            'hosts': {
                'host1': get_sqlserver_compute_host(geo='geo1'),
                'host2': get_sqlserver_compute_host(geo='geo2'),
                'host3': get_sqlserver_compute_host(geo='geo3'),
            },
            'restore-from': 'test-restore-from',
            's3_bucket': 'test-s3-bucket',
            'target-pillar-id': 'test-target-pillar-id',
        },
        get_state(),
    )


def test_sqlserver_cluster_restore_mlock_usage(mocker):
    """
    Check restore mlock usage
    """
    check_mlock_usage(
        mocker,
        'sqlserver_cluster_restore',
        {
            'hosts': {
                'host1': get_sqlserver_compute_host(geo='geo1'),
                'host2': get_sqlserver_compute_host(geo='geo2'),
                'host3': get_sqlserver_compute_host(geo='geo3'),
            },
            'restore-from': 'test-restore-from',
            's3_bucket': 'test-s3-bucket',
            'target-pillar-id': 'test-target-pillar-id',
        },
        get_state(),
    )
