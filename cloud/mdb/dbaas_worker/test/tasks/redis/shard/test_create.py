"""
Redis shard create tests
"""

from test.mocks import checked_run_task_with_mocks
from test.tasks.redis.utils import get_redis_compute_host, get_redis_porto_host
from test.tasks.utils import (
    check_mlock_usage,
    check_rejected,
    check_task_interrupt_consistency,
)


def test_porto_redis_shard_create_interrupt_consistency(mocker):
    """
    Check porto shard create interruptions
    """
    args = {
        'hosts': {
            'host1': get_redis_porto_host(geo='geo1', shard_id='test-shard'),
            'host2': get_redis_porto_host(geo='geo2', shard_id='test-shard'),
            'host3': get_redis_porto_host(geo='geo3', shard_id='test-shard'),
        },
        'sharded': True,
    }
    *_, state = checked_run_task_with_mocks(mocker, 'redis_cluster_create', dict(**args, s3_bucket='test-s3-bucket'))

    args['shard_id'] = 'test-shard2'

    args['hosts']['host4'] = get_redis_porto_host(geo='geo1', shard_id='test-shard2')
    args['hosts']['host5'] = get_redis_porto_host(geo='geo2', shard_id='test-shard2')
    args['hosts']['host6'] = get_redis_porto_host(geo='geo3', shard_id='test-shard2')

    check_task_interrupt_consistency(
        mocker,
        'redis_shard_create',
        args,
        state,
    )


def test_compute_redis_shard_create_interrupt_consistency(mocker):
    """
    Check compute shard create interruptions
    """
    args = {
        'hosts': {
            'host1': get_redis_compute_host(geo='geo1', shard_id='test-shard'),
            'host2': get_redis_compute_host(geo='geo2', shard_id='test-shard'),
            'host3': get_redis_compute_host(geo='geo3', shard_id='test-shard'),
        },
        'sharded': True,
    }
    *_, state = checked_run_task_with_mocks(mocker, 'redis_cluster_create', dict(**args, s3_bucket='test-s3-bucket'))

    args['shard_id'] = 'test-shard2'

    args['hosts']['host4'] = get_redis_compute_host(geo='geo1', shard_id='test-shard2')
    args['hosts']['host5'] = get_redis_compute_host(geo='geo2', shard_id='test-shard2')
    args['hosts']['host6'] = get_redis_compute_host(geo='geo3', shard_id='test-shard2')

    check_task_interrupt_consistency(
        mocker,
        'redis_shard_create',
        args,
        state,
    )


def test_redis_shard_create_mlock_usage(mocker):
    """
    Check shard create mlock usage
    """
    args = {
        'hosts': {
            'host1': get_redis_porto_host(geo='geo1', shard_id='test-shard'),
            'host2': get_redis_porto_host(geo='geo2', shard_id='test-shard'),
            'host3': get_redis_porto_host(geo='geo3', shard_id='test-shard'),
        },
        'sharded': True,
    }
    *_, state = checked_run_task_with_mocks(mocker, 'redis_cluster_create', dict(**args, s3_bucket='test-s3-bucket'))

    args['shard_id'] = 'test-shard2'

    args['hosts']['host4'] = get_redis_porto_host(geo='geo1', shard_id='test-shard2')
    args['hosts']['host5'] = get_redis_porto_host(geo='geo2', shard_id='test-shard2')
    args['hosts']['host6'] = get_redis_porto_host(geo='geo3', shard_id='test-shard2')

    check_mlock_usage(
        mocker,
        'redis_shard_create',
        args,
        state,
    )


def test_porto_redis_shard_create_revertable(mocker):
    """
    Check porto shard create revertability
    """
    args = {
        'hosts': {
            'host1': get_redis_porto_host(geo='geo1', shard_id='test-shard'),
            'host2': get_redis_porto_host(geo='geo2', shard_id='test-shard'),
            'host3': get_redis_porto_host(geo='geo3', shard_id='test-shard'),
        },
        'sharded': True,
    }
    *_, state = checked_run_task_with_mocks(mocker, 'redis_cluster_create', dict(**args, s3_bucket='test-s3-bucket'))

    args['shard_id'] = 'test-shard2'

    args['hosts']['host4'] = get_redis_porto_host(geo='geo1', shard_id='test-shard2')
    args['hosts']['host5'] = get_redis_porto_host(geo='geo2', shard_id='test-shard2')
    args['hosts']['host6'] = get_redis_porto_host(geo='geo3', shard_id='test-shard2')
    state['fail_actions'].add('deploy-v2-shipment-create-host1-state.sls')

    check_rejected(
        mocker,
        'redis_shard_create',
        args,
        state,
    )


def test_compute_redis_shard_create_revertable(mocker):
    """
    Check compute shard create revertability
    """
    args = {
        'hosts': {
            'host1': get_redis_compute_host(geo='geo1', shard_id='test-shard'),
            'host2': get_redis_compute_host(geo='geo2', shard_id='test-shard'),
            'host3': get_redis_compute_host(geo='geo3', shard_id='test-shard'),
        },
        'sharded': True,
    }
    *_, state = checked_run_task_with_mocks(mocker, 'redis_cluster_create', dict(**args, s3_bucket='test-s3-bucket'))

    args['shard_id'] = 'test-shard2'

    args['hosts']['host4'] = get_redis_compute_host(geo='geo1', shard_id='test-shard2')
    args['hosts']['host5'] = get_redis_compute_host(geo='geo2', shard_id='test-shard2')
    args['hosts']['host6'] = get_redis_compute_host(geo='geo3', shard_id='test-shard2')
    state['fail_actions'].add('deploy-v2-shipment-create-host1-state.sls')

    check_rejected(
        mocker,
        'redis_shard_create',
        args,
        state,
    )
