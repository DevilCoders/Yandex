"""
MongoDB cluster restore tests
"""

from test.mocks import get_state
from test.tasks.mongodb.utils import get_mongod_compute_host, get_mongod_porto_host
from test.tasks.utils import check_mlock_usage, check_task_interrupt_consistency


def test_porto_mongodb_cluster_restore_interrupt_consistency(mocker):
    """
    Check porto restore interruptions
    """
    check_task_interrupt_consistency(
        mocker,
        'mongodb_cluster_restore',
        {
            'hosts': {
                'host1': get_mongod_porto_host(geo='geo1'),
                'host2': get_mongod_porto_host(geo='geo2'),
                'host3': get_mongod_porto_host(geo='geo3'),
            },
            'restore-from': 'test-restore-from',
            's3_bucket': 'test-s3-bucket',
            'target-pillar-id': 'test-target-pillar-id',
        },
        get_state(),
    )


def test_compute_mongodb_cluster_restore_interrupt_consistency(mocker):
    """
    Check compute restore interruptions
    """
    check_task_interrupt_consistency(
        mocker,
        'mongodb_cluster_restore',
        {
            'hosts': {
                'host1': get_mongod_compute_host(geo='geo1'),
                'host2': get_mongod_compute_host(geo='geo2'),
                'host3': get_mongod_compute_host(geo='geo3'),
            },
            'restore-from': 'test-restore-from',
            's3_bucket': 'test-s3-bucket',
            'target-pillar-id': 'test-target-pillar-id',
        },
        get_state(),
    )


def test_mongodb_cluster_restore_mlock_usage(mocker):
    """
    Check restore mlock usage
    """
    check_mlock_usage(
        mocker,
        'mongodb_cluster_restore',
        {
            'hosts': {
                'host1': get_mongod_porto_host(geo='geo1'),
                'host2': get_mongod_porto_host(geo='geo2'),
                'host3': get_mongod_porto_host(geo='geo3'),
            },
            'restore-from': 'test-restore-from',
            's3_bucket': 'test-s3-bucket',
            'target-pillar-id': 'test-target-pillar-id',
        },
        get_state(),
    )
