"""
Redis host modify tests
"""

from test.mocks import checked_run_task_with_mocks
from test.tasks.redis.utils import get_redis_compute_host, get_redis_porto_host
from test.tasks.utils import check_mlock_usage, check_task_interrupt_consistency, check_alerts_synchronised


def test_porto_redis_host_modify_interrupt_consistency(mocker):
    """
    Check porto host modify interruptions
    """
    args = {
        'hosts': {
            'host1': get_redis_porto_host(geo='geo1'),
            'host2': get_redis_porto_host(geo='geo2'),
            'host3': get_redis_porto_host(geo='geo3'),
        },
    }
    *_, state = checked_run_task_with_mocks(mocker, 'redis_cluster_create', dict(**args, s3_bucket='test-s3-bucket'))

    args['host'] = {'fqdn': 'host3'}

    check_task_interrupt_consistency(
        mocker,
        'redis_host_modify',
        args,
        state,
    )


def test_compute_redis_host_modify_interrupt_consistency(mocker):
    """
    Check compute host modify interruptions
    """
    args = {
        'hosts': {
            'host1': get_redis_compute_host(geo='geo1'),
            'host2': get_redis_compute_host(geo='geo2'),
            'host3': get_redis_compute_host(geo='geo3'),
        },
    }
    *_, state = checked_run_task_with_mocks(mocker, 'redis_cluster_create', dict(**args, s3_bucket='test-s3-bucket'))

    args['host'] = {'fqdn': 'host3'}

    check_task_interrupt_consistency(
        mocker,
        'redis_host_modify',
        args,
        state,
    )


def test_redis_host_modify_alert_sync(mocker):
    """
    Check host modify mlock usage
    """
    args = {
        'hosts': {
            'host1': get_redis_porto_host(geo='geo1'),
            'host2': get_redis_porto_host(geo='geo2'),
            'host3': get_redis_porto_host(geo='geo3'),
        },
    }
    *_, state = checked_run_task_with_mocks(mocker, 'redis_cluster_create', dict(**args, s3_bucket='test-s3-bucket'))

    args['host'] = {'fqdn': 'host3'}

    check_alerts_synchronised(
        mocker,
        'redis_host_modify',
        args,
        state,
    )


def test_redis_host_modify_mlock_usage(mocker):
    """
    Check host modify mlock usage
    """
    args = {
        'hosts': {
            'host1': get_redis_porto_host(geo='geo1'),
            'host2': get_redis_porto_host(geo='geo2'),
            'host3': get_redis_porto_host(geo='geo3'),
        },
    }
    *_, state = checked_run_task_with_mocks(mocker, 'redis_cluster_create', dict(**args, s3_bucket='test-s3-bucket'))

    args['host'] = {'fqdn': 'host3'}

    check_mlock_usage(
        mocker,
        'redis_host_modify',
        args,
        state,
    )
