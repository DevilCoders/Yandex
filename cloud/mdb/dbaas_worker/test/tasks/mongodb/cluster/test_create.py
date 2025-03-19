"""
MongoDB cluster create tests
"""

from test.mocks import get_state
from test.tasks.mongodb.utils import get_mongod_compute_host, get_mongod_porto_host
from test.tasks.utils import check_mlock_usage, check_task_interrupt_consistency, check_alerts_synchronised


def test_porto_mongodb_cluster_create_interrupt_consistency(mocker):
    """
    Check porto create interruptions
    """
    check_task_interrupt_consistency(
        mocker,
        'mongodb_cluster_create',
        {
            'hosts': {
                'host1': get_mongod_porto_host(geo='geo1'),
                'host2': get_mongod_porto_host(geo='geo2'),
                'host3': get_mongod_porto_host(geo='geo3'),
            },
            's3_bucket': 'test-s3-bucket',
        },
        get_state(),
    )


def test_compute_mongodb_cluster_create_interrupt_consistency(mocker):
    """
    Check compute create interruptions
    """
    check_task_interrupt_consistency(
        mocker,
        'mongodb_cluster_create',
        {
            'hosts': {
                'host1': get_mongod_compute_host(geo='geo1'),
                'host2': get_mongod_compute_host(geo='geo2'),
                'host3': get_mongod_compute_host(geo='geo3'),
            },
            's3_bucket': 'test-s3-bucket',
        },
        get_state(),
    )


def test_mongodb_cluster_create_mlock_usage(mocker):
    """
    Check create mlock usage
    """
    check_mlock_usage(
        mocker,
        'mongodb_cluster_create',
        {
            'hosts': {
                'host1': get_mongod_porto_host(geo='geo1'),
                'host2': get_mongod_porto_host(geo='geo2'),
                'host3': get_mongod_porto_host(geo='geo3'),
            },
            's3_bucket': 'test-s3-bucket',
        },
        get_state(),
    )


def test_mongodb_cluster_create_alert_sync(mocker):
    """
    Check create mlock usage
    """
    check_alerts_synchronised(
        mocker,
        'mongodb_cluster_create',
        {
            'hosts': {
                'host1': get_mongod_porto_host(geo='geo1'),
                'host2': get_mongod_porto_host(geo='geo2'),
                'host3': get_mongod_porto_host(geo='geo3'),
            },
            's3_bucket': 'test-s3-bucket',
        },
        get_state(),
    )
