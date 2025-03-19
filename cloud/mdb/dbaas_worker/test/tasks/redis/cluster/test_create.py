"""
Redis cluster create tests
"""

from test.mocks import get_state
from test.tasks.redis.utils import get_redis_compute_host, get_redis_porto_host
from test.tasks.utils import check_mlock_usage, check_task_interrupt_consistency, check_alerts_synchronised


def test_porto_redis_cluster_create_interrupt_consistency(mocker):
    """
    Check porto create interruptions
    """
    check_task_interrupt_consistency(
        mocker,
        'redis_cluster_create',
        {
            'hosts': {
                'host1': get_redis_porto_host(geo='geo1'),
                'host2': get_redis_porto_host(geo='geo2'),
                'host3': get_redis_porto_host(geo='geo3'),
            },
            's3_bucket': 'test-s3-bucket',
            'sharded': True,
        },
        get_state(),
    )


def test_compute_redis_cluster_create_interrupt_consistency(mocker):
    """
    Check compute create interruptions
    """
    check_task_interrupt_consistency(
        mocker,
        'redis_cluster_create',
        {
            'hosts': {
                'host1': get_redis_compute_host(geo='geo1'),
                'host2': get_redis_compute_host(geo='geo2'),
                'host3': get_redis_compute_host(geo='geo3'),
            },
            's3_bucket': 'test-s3-bucket',
            'sharded': True,
        },
        get_state(),
    )


def test_redis_cluster_create_alert_sync(mocker):
    """
    Check create mlock usage
    """
    check_alerts_synchronised(
        mocker,
        'redis_cluster_create',
        {
            'hosts': {
                'host1': get_redis_porto_host(geo='geo1'),
                'host2': get_redis_porto_host(geo='geo2'),
                'host3': get_redis_porto_host(geo='geo3'),
            },
            's3_bucket': 'test-s3-bucket',
            'sharded': True,
        },
        get_state(),
    )


def test_redis_cluster_create_mlock_usage(mocker):
    """
    Check create mlock usage
    """
    check_mlock_usage(
        mocker,
        'redis_cluster_create',
        {
            'hosts': {
                'host1': get_redis_porto_host(geo='geo1'),
                'host2': get_redis_porto_host(geo='geo2'),
                'host3': get_redis_porto_host(geo='geo3'),
            },
            's3_bucket': 'test-s3-bucket',
            'sharded': True,
        },
        get_state(),
    )
