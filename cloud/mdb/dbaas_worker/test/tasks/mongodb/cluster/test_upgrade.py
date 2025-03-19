"""
MongoDB cluster upgrade tests
"""

from test.mocks import checked_run_task_with_mocks
from test.tasks.mongodb.utils import (
    get_mongocfg_compute_host,
    get_mongocfg_porto_host,
    get_mongod_compute_host,
    get_mongod_porto_host,
    get_mongos_compute_host,
    get_mongos_porto_host,
    get_mongoinfra_compute_host,
    get_mongoinfra_porto_host,
)
from test.tasks.utils import check_mlock_usage, check_task_interrupt_consistency


def test_porto_mongodb_cluster_upgrade_interrupt_consistency(mocker):
    """
    Check porto upgrade interruptions
    """
    args = {
        'hosts': {
            'host1': get_mongod_porto_host(geo='geo1'),
            'host2': get_mongod_porto_host(geo='geo2'),
            'host3': get_mongod_porto_host(geo='geo3'),
        },
    }
    *_, state = checked_run_task_with_mocks(mocker, 'mongodb_cluster_create', dict(**args, s3_bucket='test-s3-bucket'))

    check_task_interrupt_consistency(
        mocker,
        'mongodb_cluster_upgrade',
        args,
        state,
    )


def test_porto_mongodb_cluster_sharded_upgrade_interrupt_consistency(mocker):
    """
    Check porto sharded upgrade interruptions
    """
    args = {
        'hosts': {
            'host1': get_mongod_porto_host(geo='geo1'),
            'host2': get_mongod_porto_host(geo='geo2'),
            'host3': get_mongod_porto_host(geo='geo3'),
        },
    }
    *_, state = checked_run_task_with_mocks(mocker, 'mongodb_cluster_create', dict(**args, s3_bucket='test-s3-bucket'))

    args['shard_id'] = 'test-shard'

    args['hosts']['host1']['shard_id'] = 'test-shard'
    args['hosts']['host2']['shard_id'] = 'test-shard'
    args['hosts']['host3']['shard_id'] = 'test-shard'
    args['hosts']['host4'] = get_mongocfg_porto_host(geo='geo1')
    args['hosts']['host5'] = get_mongocfg_porto_host(geo='geo2')
    args['hosts']['host6'] = get_mongoinfra_porto_host(geo='geo3')
    args['hosts']['host7'] = get_mongos_porto_host(geo='geo1')

    *_, state = checked_run_task_with_mocks(mocker, 'mongodb_cluster_enable_sharding', args, state=state)

    del args['shard_id']

    check_task_interrupt_consistency(
        mocker,
        'mongodb_cluster_upgrade',
        args,
        state,
    )


def test_compute_mongodb_cluster_upgrade_interrupt_consistency(mocker):
    """
    Check compute upgrade interruptions
    """
    args = {
        'hosts': {
            'host1': get_mongod_compute_host(geo='geo1'),
            'host2': get_mongod_compute_host(geo='geo2'),
            'host3': get_mongod_compute_host(geo='geo3'),
        },
    }
    *_, state = checked_run_task_with_mocks(mocker, 'mongodb_cluster_create', dict(**args, s3_bucket='test-s3-bucket'))

    check_task_interrupt_consistency(
        mocker,
        'mongodb_cluster_upgrade',
        args,
        state,
    )


def test_compute_mongodb_cluster_sharded_upgrade_interrupt_consistency(mocker):
    """
    Check compute sharded upgrade interruptions
    """
    args = {
        'hosts': {
            'host1': get_mongod_compute_host(geo='geo1'),
            'host2': get_mongod_compute_host(geo='geo2'),
            'host3': get_mongod_compute_host(geo='geo3'),
        },
    }
    *_, state = checked_run_task_with_mocks(mocker, 'mongodb_cluster_create', dict(**args, s3_bucket='test-s3-bucket'))

    args['shard_id'] = 'test-shard'

    args['hosts']['host1']['shard_id'] = 'test-shard'
    args['hosts']['host2']['shard_id'] = 'test-shard'
    args['hosts']['host3']['shard_id'] = 'test-shard'
    args['hosts']['host4'] = get_mongocfg_compute_host(geo='geo1')
    args['hosts']['host5'] = get_mongocfg_compute_host(geo='geo2')
    args['hosts']['host6'] = get_mongoinfra_compute_host(geo='geo3')
    args['hosts']['host7'] = get_mongos_compute_host(geo='geo1')

    *_, state = checked_run_task_with_mocks(mocker, 'mongodb_cluster_enable_sharding', args, state=state)

    del args['shard_id']

    check_task_interrupt_consistency(
        mocker,
        'mongodb_cluster_upgrade',
        args,
        state,
    )


def test_mongodb_cluster_upgrade_mlock_usage(mocker):
    """
    Check upgrade mlock usage
    """
    args = {
        'hosts': {
            'host1': get_mongod_porto_host(geo='geo1'),
            'host2': get_mongod_porto_host(geo='geo2'),
            'host3': get_mongod_porto_host(geo='geo3'),
        },
    }
    *_, state = checked_run_task_with_mocks(mocker, 'mongodb_cluster_create', dict(**args, s3_bucket='test-s3-bucket'))

    check_mlock_usage(
        mocker,
        'mongodb_cluster_upgrade',
        args,
        state,
    )
