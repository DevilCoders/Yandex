"""
MySQL cluster restore tests
"""

from test.mocks import get_state
from test.tasks.mysql.utils import get_mysql_compute_host, get_mysql_porto_host
from test.tasks.utils import check_mlock_usage, check_task_interrupt_consistency


def test_porto_mysql_cluster_restore_interrupt_consistency(mocker):
    """
    Check porto restore interruptions
    """
    check_task_interrupt_consistency(
        mocker,
        'mysql_cluster_restore',
        {
            'hosts': {
                'host1': get_mysql_porto_host(geo='geo1'),
                'host2': get_mysql_porto_host(geo='geo2'),
                'host3': get_mysql_porto_host(geo='geo3'),
            },
            'restore-from': {
                'cid': 'cid-of-the-backup',
            },
            's3_bucket': 'test-s3-bucket',
            'target-pillar-id': 'test-target-pillar-id',
            'zk_hosts': 'test-zk',
        },
        get_state(),
    )


def test_compute_mysql_cluster_restore_interrupt_consistency(mocker):
    """
    Check compute restore interruptions
    """
    check_task_interrupt_consistency(
        mocker,
        'mysql_cluster_restore',
        {
            'hosts': {
                'host1': get_mysql_compute_host(geo='geo1'),
                'host2': get_mysql_compute_host(geo='geo2'),
                'host3': get_mysql_compute_host(geo='geo3'),
            },
            'restore-from': {
                'cid': 'cid-of-the-backup',
            },
            's3_bucket': 'test-s3-bucket',
            'target-pillar-id': 'test-target-pillar-id',
            'zk_hosts': 'test-zk',
        },
        get_state(),
    )


def test_mysql_cluster_restore_mlock_usage(mocker):
    """
    Check restore mlock usage
    """
    check_mlock_usage(
        mocker,
        'mysql_cluster_restore',
        {
            'hosts': {
                'host1': get_mysql_porto_host(geo='geo1'),
                'host2': get_mysql_porto_host(geo='geo2'),
                'host3': get_mysql_porto_host(geo='geo3'),
            },
            'restore-from': {
                'cid': 'cid-of-the-backup',
            },
            's3_bucket': 'test-s3-bucket',
            'target-pillar-id': 'test-target-pillar-id',
            'zk_hosts': 'test-zk',
        },
        get_state(),
    )
