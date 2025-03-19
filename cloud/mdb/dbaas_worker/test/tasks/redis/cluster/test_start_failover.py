"""
Redis cluster start failover tests
"""

from test.mocks import checked_run_task_with_mocks
from test.tasks.redis.utils import get_redis_compute_host, get_redis_porto_host
from test.tasks.utils import check_mlock_usage, check_task_interrupt_consistency


def test_porto_redis_cluster_start_failover_interrupt_consistency(mocker):
    """
    Check porto start failover interruptions
    """
    args = {
        'hosts': {
            'host1': get_redis_porto_host(geo='geo1'),
            'host2': get_redis_porto_host(geo='geo2'),
            'host3': get_redis_porto_host(geo='geo3'),
        },
    }
    *_, state = checked_run_task_with_mocks(mocker, 'redis_cluster_create', dict(**args, s3_bucket='test-s3-bucket'))

    check_task_interrupt_consistency(
        mocker,
        'redis_cluster_start_failover',
        args,
        state,
    )


def test_compute_redis_cluster_start_failover_interrupt_consistency(mocker):
    """
    Check compute start failover interruptions
    """
    args = {
        'hosts': {
            'host1': get_redis_compute_host(geo='geo1'),
            'host2': get_redis_compute_host(geo='geo2'),
            'host3': get_redis_compute_host(geo='geo3'),
        },
    }
    *_, state = checked_run_task_with_mocks(mocker, 'redis_cluster_create', dict(**args, s3_bucket='test-s3-bucket'))

    check_task_interrupt_consistency(
        mocker,
        'redis_cluster_start_failover',
        args,
        state,
    )


def test_redis_cluster_start_failover_mlock_usage(mocker):
    """
    Check start failover mlock usage
    """
    args = {
        'hosts': {
            'host1': get_redis_porto_host(geo='geo1'),
            'host2': get_redis_porto_host(geo='geo2'),
            'host3': get_redis_porto_host(geo='geo3'),
        },
    }
    *_, state = checked_run_task_with_mocks(mocker, 'redis_cluster_create', dict(**args, s3_bucket='test-s3-bucket'))

    check_mlock_usage(
        mocker,
        'redis_cluster_start_failover',
        args,
        state,
    )
