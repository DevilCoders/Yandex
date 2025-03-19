"""
MongoDB shard create tests
"""

from test.mocks import checked_run_task_with_mocks
from test.tasks.mongodb.utils import (
    get_mongocfg_compute_host,
    get_mongocfg_porto_host,
    get_mongod_compute_host,
    get_mongod_porto_host,
    get_mongoinfra_compute_host,
    get_mongoinfra_porto_host,
    get_mongos_compute_host,
    get_mongos_porto_host,
)
from test.tasks.utils import (
    check_mlock_usage,
    check_rejected,
    check_task_interrupt_consistency,
    check_alerts_synchronised,
)


def test_porto_mongodb_shard_create_interrupt_consistency(mocker):
    """
    Check porto shard create interruptions
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

    args['shard_id'] = 'test-shard2'

    args['hosts']['host9'] = get_mongod_porto_host(geo='geo1', shard_id='test-shard2')
    args['hosts']['host10'] = get_mongod_porto_host(geo='geo2', shard_id='test-shard2')
    args['hosts']['host11'] = get_mongod_porto_host(geo='geo3', shard_id='test-shard2')

    check_task_interrupt_consistency(
        mocker,
        'mongodb_shard_create',
        args,
        state,
    )


def test_compute_mongodb_shard_create_interrupt_consistency(mocker):
    """
    Check compute shard create interruptions
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

    args['shard_id'] = 'test-shard2'

    args['hosts']['host9'] = get_mongod_compute_host(geo='geo1', shard_id='test-shard2')
    args['hosts']['host10'] = get_mongod_compute_host(geo='geo2', shard_id='test-shard2')
    args['hosts']['host11'] = get_mongod_compute_host(geo='geo3', shard_id='test-shard2')

    check_task_interrupt_consistency(
        mocker,
        'mongodb_shard_create',
        args,
        state,
    )


def test_mongodb_shard_create_mlock_usage(mocker):
    """
    Check shard create mlock usage
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

    args['shard_id'] = 'test-shard2'

    args['hosts']['host9'] = get_mongod_porto_host(geo='geo1', shard_id='test-shard2')
    args['hosts']['host10'] = get_mongod_porto_host(geo='geo2', shard_id='test-shard2')
    args['hosts']['host11'] = get_mongod_porto_host(geo='geo3', shard_id='test-shard2')

    check_mlock_usage(
        mocker,
        'mongodb_shard_create',
        args,
        state,
    )

    def test_mongodb_shard_create_alert_sync(mocker):
        """
        Check shard create mlock usage
        """
        args = {
            'hosts': {
                'host1': get_mongod_porto_host(geo='geo1'),
                'host2': get_mongod_porto_host(geo='geo2'),
                'host3': get_mongod_porto_host(geo='geo3'),
            },
        }
        *_, state = checked_run_task_with_mocks(
            mocker, 'mongodb_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
        )

        args['shard_id'] = 'test-shard'

        args['hosts']['host1']['shard_id'] = 'test-shard'
        args['hosts']['host2']['shard_id'] = 'test-shard'
        args['hosts']['host3']['shard_id'] = 'test-shard'
        args['hosts']['host4'] = get_mongocfg_porto_host(geo='geo1')
        args['hosts']['host5'] = get_mongocfg_porto_host(geo='geo2')
        args['hosts']['host6'] = get_mongoinfra_porto_host(geo='geo3')
        args['hosts']['host7'] = get_mongos_porto_host(geo='geo1')

        *_, state = checked_run_task_with_mocks(mocker, 'mongodb_cluster_enable_sharding', args, state=state)

        args['shard_id'] = 'test-shard2'

        args['hosts']['host9'] = get_mongod_porto_host(geo='geo1', shard_id='test-shard2')
        args['hosts']['host10'] = get_mongod_porto_host(geo='geo2', shard_id='test-shard2')
        args['hosts']['host11'] = get_mongod_porto_host(geo='geo3', shard_id='test-shard2')

        check_alerts_synchronised(
            mocker,
            'mongodb_shard_create',
            args,
            state,
        )


def test_porto_mongodb_shard_create_revertable(mocker):
    """
    Check porto shard create revertability
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

    args['shard_id'] = 'test-shard2'

    args['hosts']['host9'] = get_mongod_porto_host(geo='geo1', shard_id='test-shard2')
    args['hosts']['host10'] = get_mongod_porto_host(geo='geo2', shard_id='test-shard2')
    args['hosts']['host11'] = get_mongod_porto_host(geo='geo3', shard_id='test-shard2')

    state['fail_actions'].add('deploy-v2-shipment-create-host9-state.highstate')

    check_rejected(
        mocker,
        'mongodb_shard_create',
        args,
        state,
    )


def test_compute_mongodb_shard_create_revertable(mocker):
    """
    Check compute shard create revertability
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

    args['shard_id'] = 'test-shard2'

    args['hosts']['host9'] = get_mongod_compute_host(geo='geo1', shard_id='test-shard2')
    args['hosts']['host10'] = get_mongod_compute_host(geo='geo2', shard_id='test-shard2')
    args['hosts']['host11'] = get_mongod_compute_host(geo='geo3', shard_id='test-shard2')

    state['fail_actions'].add('deploy-v2-shipment-create-host9-state.highstate')

    check_rejected(
        mocker,
        'mongodb_shard_create',
        args,
        state,
    )
