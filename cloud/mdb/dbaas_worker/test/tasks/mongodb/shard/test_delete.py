"""
MongoDB shard delete tests
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
from test.tasks.utils import check_mlock_usage, check_task_interrupt_consistency, check_alerts_synchronised


def test_porto_mongodb_shard_delete_interrupt_consistency(mocker):
    """
    Check porto shard delete interruptions
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

    *_, state = checked_run_task_with_mocks(mocker, 'mongodb_shard_create', args, state=state)

    args['shard_id'] = 'test-shard'
    args['shard_name'] = 'rs01'
    args['shard_hosts'] = [
        args['hosts']['host1'].copy(),
        args['hosts']['host2'].copy(),
        args['hosts']['host3'].copy(),
    ]
    args['shard_hosts'][0]['fqdn'] = 'host1'
    args['shard_hosts'][1]['fqdn'] = 'host2'
    args['shard_hosts'][2]['fqdn'] = 'host3'
    del args['hosts']['host1']
    del args['hosts']['host2']
    del args['hosts']['host3']

    check_task_interrupt_consistency(
        mocker,
        'mongodb_shard_delete',
        args,
        state,
    )


def test_compute_mongodb_shard_delete_interrupt_consistency(mocker):
    """
    Check compute shard delete interruptions
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

    *_, state = checked_run_task_with_mocks(mocker, 'mongodb_shard_create', args, state=state)

    args['shard_id'] = 'test-shard'
    args['shard_name'] = 'rs01'
    args['shard_hosts'] = [
        args['hosts']['host1'].copy(),
        args['hosts']['host2'].copy(),
        args['hosts']['host3'].copy(),
    ]
    args['shard_hosts'][0]['fqdn'] = 'host1'
    args['shard_hosts'][1]['fqdn'] = 'host2'
    args['shard_hosts'][2]['fqdn'] = 'host3'
    del args['hosts']['host1']
    del args['hosts']['host2']
    del args['hosts']['host3']

    check_task_interrupt_consistency(
        mocker,
        'mongodb_shard_delete',
        args,
        state,
        ignore=[
            # dns changes are CAS-based, so if nothing resolves nothing is deleted
            'dns.host1.db.yandex.net-AAAA-2001:db8:1::1:removed',
            'dns.host2.db.yandex.net-AAAA-2001:db8:2::2:removed',
            'dns.host3.db.yandex.net-AAAA-2001:db8:3::3:removed',
        ],
    )


def test_mongodb_shard_delete_mlock_usage(mocker):
    """
    Check shard delete mlock usage
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

    *_, state = checked_run_task_with_mocks(mocker, 'mongodb_shard_create', args, state=state)

    args['shard_id'] = 'test-shard'
    args['shard_name'] = 'rs01'
    args['shard_hosts'] = [
        args['hosts']['host1'].copy(),
        args['hosts']['host2'].copy(),
        args['hosts']['host3'].copy(),
    ]
    args['shard_hosts'][0]['fqdn'] = 'host1'
    args['shard_hosts'][1]['fqdn'] = 'host2'
    args['shard_hosts'][2]['fqdn'] = 'host3'
    del args['hosts']['host1']
    del args['hosts']['host2']
    del args['hosts']['host3']

    check_mlock_usage(
        mocker,
        'mongodb_shard_delete',
        args,
        state,
    )


def test_mongodb_shard_delete_alert_sync(mocker):
    """
    Check shard delete mlock usage
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

    *_, state = checked_run_task_with_mocks(mocker, 'mongodb_shard_create', args, state=state)

    args['shard_id'] = 'test-shard'
    args['shard_name'] = 'rs01'
    args['shard_hosts'] = [
        args['hosts']['host1'].copy(),
        args['hosts']['host2'].copy(),
        args['hosts']['host3'].copy(),
    ]
    args['shard_hosts'][0]['fqdn'] = 'host1'
    args['shard_hosts'][1]['fqdn'] = 'host2'
    args['shard_hosts'][2]['fqdn'] = 'host3'
    del args['hosts']['host1']
    del args['hosts']['host2']
    del args['hosts']['host3']

    check_alerts_synchronised(
        mocker,
        'mongodb_shard_delete',
        args,
        state,
    )
