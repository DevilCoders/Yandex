"""
ClickHouse shard delete tests
"""

from test.mocks import checked_run_task_with_mocks
from test.tasks.clickhouse.utils import (
    get_clickhouse_compute_host,
    get_clickhouse_porto_host,
    get_zookeeper_compute_host,
    get_zookeeper_porto_host,
)
from test.tasks.utils import check_mlock_usage, check_task_interrupt_consistency, check_alerts_synchronised


def test_porto_clickhouse_shard_delete_interrupt_consistency(mocker):
    """
    Check porto shard delete interruptions
    """
    args = {
        'hosts': {
            'host1': get_clickhouse_porto_host(geo='geo1', shard_id='test-shard1'),
            'host2': get_clickhouse_porto_host(geo='geo2', shard_id='test-shard1'),
            'host3': get_zookeeper_porto_host(geo='geo1'),
            'host4': get_zookeeper_porto_host(geo='geo2'),
            'host5': get_zookeeper_porto_host(geo='geo3'),
        },
    }

    *_, state = checked_run_task_with_mocks(
        mocker, 'clickhouse_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )

    args['hosts']['host6'] = get_clickhouse_porto_host(geo='geo1', shard_id='test-shard2')
    args['hosts']['host7'] = get_clickhouse_porto_host(geo='geo2', shard_id='test-shard2')

    args['shard_id'] = 'test-shard2'

    *_, state = checked_run_task_with_mocks(mocker, 'clickhouse_shard_create', args, state=state)

    del args['shard_id']
    args['zk_hosts'] = ''
    args['shard_hosts'] = [args['hosts']['host1'].copy(), args['hosts']['host2'].copy()]
    args['shard_hosts'][0]['fqdn'] = 'host1'
    args['shard_hosts'][1]['fqdn'] = 'host2'
    del args['hosts']['host1']
    del args['hosts']['host2']

    check_task_interrupt_consistency(
        mocker,
        'clickhouse_shard_delete',
        args,
        state,
    )


def test_compute_clickhouse_shard_delete_interrupt_consistency(mocker):
    """
    Check compute shard delete interruptions
    """
    args = {
        'hosts': {
            'host1': get_clickhouse_compute_host(geo='geo1', shard_id='test-shard1'),
            'host2': get_clickhouse_compute_host(geo='geo2', shard_id='test-shard1'),
            'host3': get_zookeeper_compute_host(geo='geo1'),
            'host4': get_zookeeper_compute_host(geo='geo2'),
            'host5': get_zookeeper_compute_host(geo='geo3'),
        },
    }

    *_, state = checked_run_task_with_mocks(
        mocker, 'clickhouse_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )

    args['hosts']['host6'] = get_clickhouse_compute_host(geo='geo1', shard_id='test-shard2')
    args['hosts']['host7'] = get_clickhouse_compute_host(geo='geo2', shard_id='test-shard2')

    args['shard_id'] = 'test-shard2'

    *_, state = checked_run_task_with_mocks(mocker, 'clickhouse_shard_create', args, state=state)

    del args['shard_id']
    args['zk_hosts'] = ''
    args['shard_hosts'] = [args['hosts']['host1'].copy(), args['hosts']['host2'].copy()]
    args['shard_hosts'][0]['fqdn'] = 'host1'
    args['shard_hosts'][1]['fqdn'] = 'host2'
    del args['hosts']['host1']
    del args['hosts']['host2']

    check_task_interrupt_consistency(
        mocker,
        'clickhouse_shard_delete',
        args,
        state,
        ignore=[
            # dns changes are CAS-based, so if nothing resolves nothing is deleted
            'dns.host1.db.yandex.net-AAAA-2001:db8:1::4:removed',
            'dns.host2.db.yandex.net-AAAA-2001:db8:2::5:removed',
        ],
    )


def test_clickhouse_shard_delete_mlock_usage(mocker):
    """
    Check shard delete mlock usage
    """
    args = {
        'hosts': {
            'host1': get_clickhouse_porto_host(geo='geo1', shard_id='test-shard1'),
            'host2': get_clickhouse_porto_host(geo='geo2', shard_id='test-shard1'),
            'host3': get_zookeeper_porto_host(geo='geo1'),
            'host4': get_zookeeper_porto_host(geo='geo2'),
            'host5': get_zookeeper_porto_host(geo='geo3'),
        },
    }

    *_, state = checked_run_task_with_mocks(
        mocker, 'clickhouse_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )

    args['hosts']['host6'] = get_clickhouse_porto_host(geo='geo1', shard_id='test-shard2')
    args['hosts']['host7'] = get_clickhouse_porto_host(geo='geo2', shard_id='test-shard2')

    args['shard_id'] = 'test-shard2'

    *_, state = checked_run_task_with_mocks(mocker, 'clickhouse_shard_create', args, state=state)

    del args['shard_id']
    args['zk_hosts'] = ''
    args['shard_hosts'] = [args['hosts']['host1'].copy(), args['hosts']['host2'].copy()]
    args['shard_hosts'][0]['fqdn'] = 'host1'
    args['shard_hosts'][1]['fqdn'] = 'host2'
    del args['hosts']['host1']
    del args['hosts']['host2']

    check_mlock_usage(
        mocker,
        'clickhouse_shard_delete',
        args,
        state,
    )


def test_clickhouse_shard_delete_alert_sync(mocker):
    """
    Check shard delete mlock usage
    """
    args = {
        'hosts': {
            'host1': get_clickhouse_porto_host(geo='geo1', shard_id='test-shard1'),
            'host2': get_clickhouse_porto_host(geo='geo2', shard_id='test-shard1'),
            'host3': get_zookeeper_porto_host(geo='geo1'),
            'host4': get_zookeeper_porto_host(geo='geo2'),
            'host5': get_zookeeper_porto_host(geo='geo3'),
        },
    }

    *_, state = checked_run_task_with_mocks(
        mocker, 'clickhouse_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )

    args['hosts']['host6'] = get_clickhouse_porto_host(geo='geo1', shard_id='test-shard2')
    args['hosts']['host7'] = get_clickhouse_porto_host(geo='geo2', shard_id='test-shard2')

    args['shard_id'] = 'test-shard2'

    *_, state = checked_run_task_with_mocks(mocker, 'clickhouse_shard_create', args, state=state)

    del args['shard_id']
    args['zk_hosts'] = ''
    args['shard_hosts'] = [args['hosts']['host1'].copy(), args['hosts']['host2'].copy()]
    args['shard_hosts'][0]['fqdn'] = 'host1'
    args['shard_hosts'][1]['fqdn'] = 'host2'
    del args['hosts']['host1']
    del args['hosts']['host2']

    check_alerts_synchronised(
        mocker,
        'clickhouse_shard_delete',
        args,
        state,
    )
