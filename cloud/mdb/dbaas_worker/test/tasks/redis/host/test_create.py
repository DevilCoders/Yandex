"""
Redis host create tests
"""

from test.mocks import checked_run_task_with_mocks
from test.tasks.redis.utils import get_redis_compute_host, get_redis_porto_host
from test.tasks.utils import (
    check_mlock_usage,
    check_rejected,
    check_task_interrupt_consistency,
)


def test_porto_redis_host_create_interrupt_consistency(mocker):
    """
    Check porto host create interruptions
    """
    args = {
        'hosts': {
            'host1': get_redis_porto_host(geo='geo1'),
            'host2': get_redis_porto_host(geo='geo2'),
            'host3': get_redis_porto_host(geo='geo3'),
        },
    }
    *_, state = checked_run_task_with_mocks(mocker, 'redis_cluster_create', dict(**args, s3_bucket='test-s3-bucket'))

    args['hosts']['host4'] = get_redis_porto_host(geo='geo1')
    args['host'] = 'host4'

    check_task_interrupt_consistency(
        mocker,
        'redis_host_create',
        args,
        state,
    )


def test_porto_redis_shard_host_create_interrupt_consistency(mocker):
    """
    Check porto shard host create interruptions
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

    args['hosts']['host4'] = get_redis_porto_host(geo='geo1')
    args['host'] = 'host4'

    check_task_interrupt_consistency(
        mocker,
        'redis_shard_host_create',
        args,
        state,
    )


def test_compute_redis_host_create_interrupt_consistency(mocker):
    """
    Check compute host create interruptions
    """
    args = {
        'hosts': {
            'host1': get_redis_compute_host(geo='geo1'),
            'host2': get_redis_compute_host(geo='geo2'),
            'host3': get_redis_compute_host(geo='geo3'),
        },
    }
    *_, state = checked_run_task_with_mocks(mocker, 'redis_cluster_create', dict(**args, s3_bucket='test-s3-bucket'))

    args['hosts']['host4'] = get_redis_compute_host(geo='geo1')
    args['host'] = 'host4'

    check_task_interrupt_consistency(
        mocker,
        'redis_host_create',
        args,
        state,
    )


def test_compute_redis_shard_host_create_interrupt_consistency(mocker):
    """
    Check compute shard host create interruptions
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

    args['hosts']['host4'] = get_redis_compute_host(geo='geo1')
    args['host'] = 'host4'

    check_task_interrupt_consistency(
        mocker,
        'redis_shard_host_create',
        args,
        state,
    )


def test_redis_host_create_mlock_usage(mocker):
    """
    Check host create mlock usage
    """
    args = {
        'hosts': {
            'host1': get_redis_porto_host(geo='geo1'),
            'host2': get_redis_porto_host(geo='geo2'),
            'host3': get_redis_porto_host(geo='geo3'),
        },
    }
    *_, state = checked_run_task_with_mocks(mocker, 'redis_cluster_create', dict(**args, s3_bucket='test-s3-bucket'))

    args['hosts']['host4'] = get_redis_porto_host(geo='geo1')
    args['host'] = 'host4'

    check_mlock_usage(
        mocker,
        'redis_host_create',
        args,
        state,
    )


def test_redis_shard_host_create_mlock_usage(mocker):
    """
    Check shard host create mlock usage
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

    args['hosts']['host4'] = get_redis_porto_host(geo='geo1')
    args['host'] = 'host4'

    check_mlock_usage(
        mocker,
        'redis_shard_host_create',
        args,
        state,
    )


def test_porto_redis_host_create_revertable(mocker):
    """
    Check porto host create revertability
    """
    args = {
        'hosts': {
            'host1': get_redis_porto_host(geo='geo1'),
            'host2': get_redis_porto_host(geo='geo2'),
            'host3': get_redis_porto_host(geo='geo3'),
        },
    }
    *_, state = checked_run_task_with_mocks(mocker, 'redis_cluster_create', dict(**args, s3_bucket='test-s3-bucket'))

    args['hosts']['host4'] = get_redis_porto_host(geo='geo1')
    args['host'] = 'host4'
    state['fail_actions'].add('deploy-v2-shipment-create-host1-state.sls')

    check_rejected(
        mocker,
        'redis_host_create',
        args,
        state,
    )


def test_porto_redis_shard_host_create_revertable(mocker):
    """
    Check porto host create revertability
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

    args['hosts']['host4'] = get_redis_porto_host(geo='geo1')
    args['host'] = 'host4'
    state['fail_actions'].add('deploy-v2-shipment-create-host1-state.sls')

    check_rejected(
        mocker,
        'redis_shard_host_create',
        args,
        state,
    )


def test_compute_redis_host_create_revertable(mocker):
    """
    Check compute host create revertability
    """
    args = {
        'hosts': {
            'host1': get_redis_compute_host(geo='geo1'),
            'host2': get_redis_compute_host(geo='geo2'),
            'host3': get_redis_compute_host(geo='geo3'),
        },
    }
    *_, state = checked_run_task_with_mocks(mocker, 'redis_cluster_create', dict(**args, s3_bucket='test-s3-bucket'))

    args['hosts']['host4'] = get_redis_compute_host(geo='geo1')
    args['host'] = 'host4'
    state['fail_actions'].add('deploy-v2-shipment-create-host1-state.sls')

    check_rejected(
        mocker,
        'redis_host_create',
        args,
        state,
    )


def test_compute_redis_shard_host_create_revertable(mocker):
    """
    Check compute host create revertability
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

    args['hosts']['host4'] = get_redis_compute_host(geo='geo1')
    args['host'] = 'host4'
    state['fail_actions'].add('deploy-v2-shipment-create-host1-state.sls')

    check_rejected(
        mocker,
        'redis_shard_host_create',
        args,
        state,
    )
