"""
MongoDB host create tests
"""

from test.mocks import checked_run_task_with_mocks
from test.tasks.mongodb.utils import get_mongod_compute_host, get_mongod_porto_host
from test.tasks.utils import (
    check_mlock_usage,
    check_rejected,
    check_task_interrupt_consistency,
)


def test_porto_mongodb_host_create_interrupt_consistency(mocker):
    """
    Check porto host create interruptions
    """
    args = {
        'hosts': {
            'host1': get_mongod_porto_host(geo='geo1'),
            'host2': get_mongod_porto_host(geo='geo2'),
            'host3': get_mongod_porto_host(geo='geo3'),
        },
    }
    *_, state = checked_run_task_with_mocks(mocker, 'mongodb_cluster_create', dict(**args, s3_bucket='test-s3-bucket'))

    args['hosts']['host4'] = get_mongod_porto_host(geo='geo1')
    args['host'] = 'host4'
    args['subcid'] = args['hosts']['host4']['subcid']
    args['shard_id'] = args['hosts']['host4']['shard_id']

    check_task_interrupt_consistency(
        mocker,
        'mongodb_host_create',
        args,
        state,
    )


def test_compute_mongodb_host_create_interrupt_consistency(mocker):
    """
    Check compute host create interruptions
    """
    args = {
        'hosts': {
            'host1': get_mongod_compute_host(geo='geo1'),
            'host2': get_mongod_compute_host(geo='geo2'),
            'host3': get_mongod_compute_host(geo='geo3'),
        },
    }
    *_, state = checked_run_task_with_mocks(mocker, 'mongodb_cluster_create', dict(**args, s3_bucket='test-s3-bucket'))

    args['hosts']['host4'] = get_mongod_compute_host(geo='geo1')
    args['host'] = 'host4'
    args['subcid'] = args['hosts']['host4']['subcid']
    args['shard_id'] = args['hosts']['host4']['shard_id']

    check_task_interrupt_consistency(
        mocker,
        'mongodb_host_create',
        args,
        state,
    )


def test_mongodb_host_create_mlock_usage(mocker):
    """
    Check host create mlock usage
    """
    args = {
        'hosts': {
            'host1': get_mongod_porto_host(geo='geo1'),
            'host2': get_mongod_porto_host(geo='geo2'),
            'host3': get_mongod_porto_host(geo='geo3'),
        },
    }
    *_, state = checked_run_task_with_mocks(mocker, 'mongodb_cluster_create', dict(**args, s3_bucket='test-s3-bucket'))

    args['hosts']['host4'] = get_mongod_porto_host(geo='geo1')
    args['host'] = 'host4'
    args['subcid'] = args['hosts']['host4']['subcid']
    args['shard_id'] = args['hosts']['host4']['shard_id']

    check_mlock_usage(
        mocker,
        'mongodb_host_create',
        args,
        state,
    )


def test_porto_mongodb_host_create_revertable(mocker):
    """
    Check porto host create revertability
    """
    args = {
        'hosts': {
            'host1': get_mongod_porto_host(geo='geo1'),
            'host2': get_mongod_porto_host(geo='geo2'),
            'host3': get_mongod_porto_host(geo='geo3'),
        },
    }
    *_, state = checked_run_task_with_mocks(mocker, 'mongodb_cluster_create', dict(**args, s3_bucket='test-s3-bucket'))

    args['hosts']['host4'] = get_mongod_porto_host(geo='geo1')
    args['host'] = 'host4'
    args['subcid'] = args['hosts']['host4']['subcid']
    args['shard_id'] = args['hosts']['host4']['shard_id']
    state['fail_actions'].add('deploy-v2-shipment-create-host1-state.sls')

    check_rejected(
        mocker,
        'mongodb_host_create',
        args,
        state,
    )


def test_compute_mongodb_host_create_revertable(mocker):
    """
    Check compute host create revertability
    """
    args = {
        'hosts': {
            'host1': get_mongod_compute_host(geo='geo1'),
            'host2': get_mongod_compute_host(geo='geo2'),
            'host3': get_mongod_compute_host(geo='geo3'),
        },
    }
    *_, state = checked_run_task_with_mocks(mocker, 'mongodb_cluster_create', dict(**args, s3_bucket='test-s3-bucket'))

    args['hosts']['host4'] = get_mongod_compute_host(geo='geo1')
    args['host'] = 'host4'
    args['subcid'] = args['hosts']['host4']['subcid']
    args['shard_id'] = args['hosts']['host4']['shard_id']
    state['fail_actions'].add('deploy-v2-shipment-create-host1-state.sls')

    check_rejected(
        mocker,
        'mongodb_host_create',
        args,
        state,
    )
