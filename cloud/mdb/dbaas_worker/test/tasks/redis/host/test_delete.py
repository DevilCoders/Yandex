"""
Redis host delete tests
"""

from test.mocks import checked_run_task_with_mocks
from test.tasks.redis.utils import get_redis_compute_host, get_redis_porto_host
from test.tasks.utils import check_mlock_usage, check_task_interrupt_consistency, check_alerts_synchronised


def test_porto_redis_host_delete_interrupt_consistency(mocker):
    """
    Check porto host delete interruptions
    """
    args = {
        'hosts': {
            'host1': get_redis_porto_host(geo='geo1'),
            'host2': get_redis_porto_host(geo='geo2'),
            'host3': get_redis_porto_host(geo='geo3'),
        },
    }
    *_, state = checked_run_task_with_mocks(mocker, 'redis_cluster_create', dict(**args, s3_bucket='test-s3-bucket'))

    args['host'] = args['hosts']['host3'].copy()
    args['host']['fqdn'] = 'host3'
    del args['hosts']['host3']

    check_task_interrupt_consistency(
        mocker,
        'redis_host_delete',
        args,
        state,
    )


def test_porto_redis_shard_host_delete_interrupt_consistency(mocker):
    """
    Check porto shard host delete interruptions
    """
    args = {
        'hosts': {
            'host1': get_redis_porto_host(geo='geo1'),
            'host2': get_redis_porto_host(geo='geo2'),
            'host3': get_redis_porto_host(geo='geo3'),
        },
        'sharded': True,
    }
    *_, state = checked_run_task_with_mocks(mocker, 'redis_cluster_create', dict(**args, s3_bucket='test-s3-bucket'))

    args['host'] = args['hosts']['host3'].copy()
    args['host']['fqdn'] = 'host3'
    del args['hosts']['host3']

    check_task_interrupt_consistency(
        mocker,
        'redis_shard_host_delete',
        args,
        state,
    )


def test_compute_redis_host_delete_interrupt_consistency(mocker):
    """
    Check compute host delete interruptions
    """
    args = {
        'hosts': {
            'host1': get_redis_compute_host(geo='geo1'),
            'host2': get_redis_compute_host(geo='geo2'),
            'host3': get_redis_compute_host(geo='geo3'),
        },
    }
    *_, state = checked_run_task_with_mocks(mocker, 'redis_cluster_create', dict(**args, s3_bucket='test-s3-bucket'))

    args['host'] = args['hosts']['host3'].copy()
    args['host']['fqdn'] = 'host3'
    del args['hosts']['host3']

    check_task_interrupt_consistency(
        mocker,
        'redis_host_delete',
        args,
        state,
        ignore=[
            # dns changes are CAS-based, so if nothing resolves nothing is deleted
            'dns.host3.db.yandex.net-AAAA-2001:db8:3::3:removed',
        ],
    )


def test_compute_redis_shard_host_delete_interrupt_consistency(mocker):
    """
    Check compute shard host delete interruptions
    """
    args = {
        'hosts': {
            'host1': get_redis_compute_host(geo='geo1'),
            'host2': get_redis_compute_host(geo='geo2'),
            'host3': get_redis_compute_host(geo='geo3'),
        },
        'sharded': True,
    }
    *_, state = checked_run_task_with_mocks(mocker, 'redis_cluster_create', dict(**args, s3_bucket='test-s3-bucket'))

    args['host'] = args['hosts']['host3'].copy()
    args['host']['fqdn'] = 'host3'
    del args['hosts']['host3']

    check_task_interrupt_consistency(
        mocker,
        'redis_shard_host_delete',
        args,
        state,
        ignore=[
            # dns changes are CAS-based, so if nothing resolves nothing is deleted
            'dns.host3.db.yandex.net-AAAA-2001:db8:3::3:removed',
        ],
    )


def test_redis_host_delete_alert_sync(mocker):
    """
    Check host delete mlock usage
    """
    args = {
        'hosts': {
            'host1': get_redis_porto_host(geo='geo1'),
            'host2': get_redis_porto_host(geo='geo2'),
            'host3': get_redis_porto_host(geo='geo3'),
        },
    }
    *_, state = checked_run_task_with_mocks(mocker, 'redis_cluster_create', dict(**args, s3_bucket='test-s3-bucket'))

    args['host'] = args['hosts']['host3'].copy()
    args['host']['fqdn'] = 'host3'
    del args['hosts']['host3']

    check_alerts_synchronised(
        mocker,
        'redis_host_delete',
        args,
        state,
    )


def test_redis_host_delete_mlock_usage(mocker):
    """
    Check host delete mlock usage
    """
    args = {
        'hosts': {
            'host1': get_redis_porto_host(geo='geo1'),
            'host2': get_redis_porto_host(geo='geo2'),
            'host3': get_redis_porto_host(geo='geo3'),
        },
    }
    *_, state = checked_run_task_with_mocks(mocker, 'redis_cluster_create', dict(**args, s3_bucket='test-s3-bucket'))

    args['host'] = args['hosts']['host3'].copy()
    args['host']['fqdn'] = 'host3'
    del args['hosts']['host3']

    check_mlock_usage(
        mocker,
        'redis_host_delete',
        args,
        state,
    )


def test_redis_shard_host_delete_mlock_usage(mocker):
    """
    Check shard host delete mlock usage
    """
    args = {
        'hosts': {
            'host1': get_redis_porto_host(geo='geo1'),
            'host2': get_redis_porto_host(geo='geo2'),
            'host3': get_redis_porto_host(geo='geo3'),
        },
        'sharded': True,
    }
    *_, state = checked_run_task_with_mocks(mocker, 'redis_cluster_create', dict(**args, s3_bucket='test-s3-bucket'))

    args['host'] = args['hosts']['host3'].copy()
    args['host']['fqdn'] = 'host3'
    del args['hosts']['host3']

    check_mlock_usage(
        mocker,
        'redis_shard_host_delete',
        args,
        state,
    )
