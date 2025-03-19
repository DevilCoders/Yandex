"""
ClickHouse shard modify tests
"""

from test.mocks import checked_run_task_with_mocks
from test.tasks.clickhouse.utils import (
    get_clickhouse_compute_host,
    get_clickhouse_porto_host,
    get_zookeeper_compute_host,
    get_zookeeper_porto_host,
)
from test.tasks.utils import check_mlock_usage, check_task_interrupt_consistency, check_alerts_synchronised


def test_porto_clickhouse_shard_modify_interrupt_consistency(mocker):
    """
    Check porto shard modify interruptions
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
    args['hosts']['host8'] = get_clickhouse_porto_host(geo='geo3', shard_id='test-shard2')

    args['shard_id'] = 'test-shard2'

    *_, state = checked_run_task_with_mocks(mocker, 'clickhouse_shard_create', args, state=state)

    # to cause move
    args['hosts']['host6']['platform_id'] = '2'

    # to cause in-place resize
    args['hosts']['host7']['cpu_limit'] = args['hosts']['host7']['cpu_limit'] * 2

    # to cause volume resize
    args['hosts']['host8']['space_limit'] = args['hosts']['host8']['space_limit'] * 2

    args['restart'] = True

    check_task_interrupt_consistency(
        mocker,
        'clickhouse_shard_modify',
        args,
        state,
        # we skip pre-restart on already moving container
        ignore=['deployv2.components.dbaas-operations.run-pre-restart-script.host6.host6-state.sls:started'],
    )


def test_compute_clickhouse_shard_modify_interrupt_consistency(mocker):
    """
    Check compute shard modify interruptions
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

    args['hosts']['host6'] = get_clickhouse_compute_host(
        geo='geo1', shard_id='test-shard2', disk_type_id='local-ssd', space_limit=107374182400
    )
    args['hosts']['host7'] = get_clickhouse_compute_host(geo='geo2', shard_id='test-shard2')
    args['hosts']['host8'] = get_clickhouse_compute_host(geo='geo2', shard_id='test-shard2')

    args['shard_id'] = 'test-shard2'

    *_, state = checked_run_task_with_mocks(mocker, 'clickhouse_shard_create', args, state=state)

    # to cause local disk resize
    args['hosts']['host6']['space_limit'] = args['hosts']['host6']['space_limit'] * 2

    # to cause network disk resize and resources change
    args['hosts']['host7']['cpu_limit'] = args['hosts']['host7']['cpu_limit'] * 2
    args['hosts']['host7']['space_limit'] = args['hosts']['host7']['space_limit'] * 2

    # to cause network disk downscale
    args['hosts']['host8']['space_limit'] = args['hosts']['host8']['space_limit'] // 2

    check_task_interrupt_consistency(
        mocker,
        'clickhouse_shard_modify',
        args,
        state,
        # local ssd resize causes instance delete
        # restore after interruption after it leaves out some changes
        ignore=[
            'host6.disks_listed:True',
            'disk.disk-16:create initiated',
            'instance-6.disk.disk-16:attach initiated',
            'deployv2.components.dbaas-operations.run-data-move-front-script.host6.host6-state.sls:started',
            'instance-6.disk.disk-16:detach initiated',
            'instance-6.disk-11.autodelete.False:set initiated',
            'host6-disks-save-before-delete:True',
            'instance.host6:delete',
            'metadb_host.host6.vtype_id:updated',
            'instance.host6:stop initiated',
            'dns.host6.db.yandex.net-AAAA-2001:db8:1::6:removed',
            'instance.instance-6:stop initiated',
        ],
    )


def test_clickhouse_shard_modify_mlock_usage(mocker):
    """
    Check shard modify mlock usage
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
    args['hosts']['host8'] = get_clickhouse_porto_host(geo='geo3', shard_id='test-shard2')

    args['shard_id'] = 'test-shard2'

    *_, state = checked_run_task_with_mocks(mocker, 'clickhouse_shard_create', args, state=state)

    args['restart'] = True

    check_mlock_usage(
        mocker,
        'clickhouse_shard_modify',
        args,
        state,
    )


def test_clickhouse_shard_modify_alert_sync(mocker):
    """
    Check shard modify mlock usage
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
    args['hosts']['host8'] = get_clickhouse_porto_host(geo='geo3', shard_id='test-shard2')

    args['shard_id'] = 'test-shard2'

    *_, state = checked_run_task_with_mocks(mocker, 'clickhouse_shard_create', args, state=state)

    args['restart'] = True

    check_alerts_synchronised(
        mocker,
        'clickhouse_shard_modify',
        args,
        state,
    )
