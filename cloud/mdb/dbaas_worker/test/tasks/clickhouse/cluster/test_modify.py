"""
ClickHouse cluster modify tests
"""

from hamcrest import assert_that, has_entries, has_items

from test.mocks import checked_run_task_with_mocks
from test.tasks.clickhouse.utils import (
    get_clickhouse_compute_host,
    get_clickhouse_porto_host,
    get_zookeeper_compute_host,
    get_zookeeper_porto_host,
)
from test.tasks.utils import check_mlock_usage, check_task_interrupt_consistency, check_alerts_synchronised

from cloud.mdb.dbaas_worker.internal.tasks.clickhouse.cluster.modify import ClickHouseClusterModify


def test_porto_clickhouse_cluster_modify_interrupt_consistency(mocker):
    """
    Check porto modify interruptions
    """
    args = {
        'hosts': {
            'host1': get_clickhouse_porto_host(geo='geo1'),
            'host2': get_clickhouse_porto_host(geo='geo2'),
            'host3': get_zookeeper_porto_host(geo='geo1'),
            'host4': get_zookeeper_porto_host(geo='geo2'),
            'host5': get_zookeeper_porto_host(geo='geo3'),
        },
    }

    *_, state = checked_run_task_with_mocks(
        mocker, 'clickhouse_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )

    # to cause move
    args['hosts']['host1']['platform_id'] = '2'

    # to cause in-place resize
    args['hosts']['host2']['cpu_limit'] = args['hosts']['host2']['cpu_limit'] * 2

    # to cause volume resize
    args['hosts']['host3']['space_limit'] = args['hosts']['host3']['space_limit'] * 2

    args['restart'] = True

    check_task_interrupt_consistency(
        mocker,
        'clickhouse_cluster_modify',
        args,
        state,
        # we skip pre-restart on already moving container
        ignore=['deployv2.components.dbaas-operations.run-pre-restart-script.host1.host1-state.sls:started'],
    )


def test_compute_clickhouse_cluster_modify_interrupt_consistency(mocker):
    """
    Check compute modify interruptions
    """
    args = {
        'hosts': {
            'host1': get_clickhouse_compute_host(geo='geo1', disk_type_id='local-ssd', space_limit=107374182400),
            'host2': get_clickhouse_compute_host(geo='geo2'),
            'host3': get_zookeeper_compute_host(geo='geo1'),
            'host4': get_zookeeper_compute_host(geo='geo2'),
            'host5': get_zookeeper_compute_host(geo='geo3'),
        },
        'service_account_id': 'test-service-account',
    }

    *_, state = checked_run_task_with_mocks(
        mocker, 'clickhouse_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )

    # to cause local disk resize
    args['hosts']['host1']['space_limit'] = args['hosts']['host1']['space_limit'] * 2

    # to cause network disk resize and resources change
    args['hosts']['host3']['cpu_limit'] = args['hosts']['host2']['cpu_limit'] * 2
    args['hosts']['host3']['space_limit'] = args['hosts']['host2']['space_limit'] * 2

    # to cause network disk downscale
    args['hosts']['host4']['space_limit'] = args['hosts']['host3']['space_limit'] // 2

    # to cause instance attribute change
    args['service_account_id'] = 'test-service-account2'

    args['restart'] = True

    interrupted_state = check_task_interrupt_consistency(
        mocker,
        'clickhouse_cluster_modify',
        args,
        state,
        # local ssd resize causes instance delete
        # restore after interruption after it leaves out some changes
        ignore=[
            'host1.disks_listed:True',
            'disk.disk-10:create initiated',
            'instance-4.disk.disk-10:attach initiated',
            'deployv2.components.dbaas-operations.run-data-move-front-script.host1.host1-state.sls:started',
            'instance-4.disk.disk-10:detach initiated',
            'instance-4.disk-7.autodelete.False:set initiated',
            'host1-disks-save-before-delete:True',
            'instance.host1:delete',
            'metadb_host.host1.vtype_id:updated',
            'instance.host1:stop initiated',
            'dns.host1.db.yandex.net-AAAA-2001:db8:1::4:removed',
            'instance.instance-4:stop initiated',
        ],
    )

    for hostname in ['host1', 'host2', 'host3', 'host4', 'host5']:
        assert_that(
            interrupted_state['compute']['instances'].values(),
            has_items(has_entries({'fqdn': hostname, 'status': 'RUNNING'})),
        )


def test_clickhouse_cluster_modify_mlock_usage(mocker):
    """
    Check modify mlock usage
    """
    args = {
        'hosts': {
            'host1': get_clickhouse_porto_host(geo='geo1'),
            'host2': get_clickhouse_porto_host(geo='geo2'),
            'host3': get_zookeeper_porto_host(geo='geo1'),
            'host4': get_zookeeper_porto_host(geo='geo2'),
            'host5': get_zookeeper_porto_host(geo='geo3'),
        },
    }

    *_, state = checked_run_task_with_mocks(
        mocker, 'clickhouse_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )

    args['restart'] = True

    check_mlock_usage(
        mocker,
        'clickhouse_cluster_modify',
        args,
        state,
    )


def test_clickhouse_cluster_modify_alert_sync(mocker):
    """
    Check modify mlock usage
    """
    args = {
        'hosts': {
            'host1': get_clickhouse_porto_host(geo='geo1'),
            'host2': get_clickhouse_porto_host(geo='geo2'),
            'host3': get_zookeeper_porto_host(geo='geo1'),
            'host4': get_zookeeper_porto_host(geo='geo2'),
            'host5': get_zookeeper_porto_host(geo='geo3'),
        },
    }

    *_, state = checked_run_task_with_mocks(
        mocker, 'clickhouse_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )

    args['restart'] = True

    check_alerts_synchronised(
        mocker,
        'clickhouse_cluster_modify',
        args,
        state,
    )


def test_clickhouse_cluster_modify_alt_conductor_group(mocker):
    """
    Check that cluster correctly changes alt conductor group on modify
    """
    args = {
        'hosts': {
            'host1': get_clickhouse_porto_host(geo='geo1', flavor='alt-test-flavor', subcid='ch-subcid'),
            'host2': get_clickhouse_porto_host(geo='geo2', flavor='alt-test-flavor', subcid='ch-subcid'),
            'host3': get_zookeeper_porto_host(geo='geo1', flavor='alt-test-flavor', subcid='zk-subcid'),
            'host4': get_zookeeper_porto_host(geo='geo2', flavor='alt-test-flavor', subcid='zk-subcid'),
            'host5': get_zookeeper_porto_host(geo='geo3', flavor='alt-test-flavor', subcid='zk-subcid'),
        },
    }
    *_, state = checked_run_task_with_mocks(
        mocker, 'clickhouse_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )

    assert (
        state['conductor']['groups']['db_ch_subcid']['parent_ids'][0]
        == state['conductor']['groups']['mdb_test_altgroup']['id']
    ), 'Unexpected ch initial group id'

    assert (
        state['conductor']['groups']['db_zk_subcid']['parent_ids'][0]
        == state['conductor']['groups']['mdb_test_altgroup']['id']
    ), 'Unexpected zk initial group id'

    for host in args['hosts'].copy():
        args['hosts'][host]['flavor'] = 'test-flavor'

    args['restart'] = True

    *_, state = checked_run_task_with_mocks(mocker, 'clickhouse_cluster_modify', args, state=state)

    assert (
        state['conductor']['groups']['db_ch_subcid']['parent_ids'][0]
        == state['conductor']['groups']['mdb_test_clickhouse']['id']
    ), 'Unexpected ch group id after modify'

    assert (
        state['conductor']['groups']['db_zk_subcid']['parent_ids'][0]
        == state['conductor']['groups']['mdb_test_zookeeper']['id']
    ), 'Unexpected zk group id after modify'


def test_clickhouse_cluster_modify_choose_keepers_to_run_hs_first():
    test_data = [
        {
            'message': 'Choose keepers from different shards',
            'args': [
                {"keeper1": True, "keeper2": False, "keeper3": False},
                {
                    "keeper1": {"shard_id": "shard1"},
                    "keeper2": {"shard_id": "shard1"},
                    "keeper3": {"shard_id": "shard2"},
                    "not_a_keeepr": {"shard_id": "shard2"},
                },
            ],
            'result': ["keeper1", "keeper3"],
        },
        {
            'message': 'Choose keepers from different shards (with 2 keepers with data)',
            'args': [
                {"keeper1": True, "keeper2": True, "keeper3": False},
                {
                    "keeper1": {"shard_id": "shard1"},
                    "keeper2": {"shard_id": "shard2"},
                    "keeper3": {"shard_id": "shard1"},
                    "not_a_keeepr": {"shard_id": "shard2"},
                },
            ],
            'result': ["keeper1", "keeper2"],
        },
        {
            'message': 'Choose keepers from one shard',
            'args': [
                {"keeper1": True, "keeper2": True, "keeper3": False},
                {
                    "keeper1": {"shard_id": "shard1"},
                    "keeper2": {"shard_id": "shard1"},
                    "keeper3": {"shard_id": "shard1"},
                },
            ],
            'result': ["keeper1", "keeper2"],
        },
    ]
    for data in test_data:
        assert sorted(ClickHouseClusterModify._choose_keepers_to_run_hs_first(*data['args'])) == sorted(
            data['result']
        ), ('Unexpected keepers choice in test - ' + data['message'])


def test_clickhouse_cluster_modify_choose_backup_host():
    test_data = [
        {
            'message': 'Select any existing host as a schema source for new shard hosts',
            'args': [
                {
                    "old_host1": {"shard_name": "shard1"},
                    "old_host2": {"shard_name": "shard1"},
                    "new_host1": {"shard_name": "shard2"},
                    "new_host2": {"shard_name": "shard2"},
                },
                ["new_host1", "new_host2"],
                "shard2",
                [{"fqdn": "new_host1"}, {"fqdn": "new_host2"}],
            ],
            'result': ("old_host1", "shard1"),
        },
        {
            'message': 'Select existing shard host as a schema source for new host in existing shard',
            'args': [
                {
                    "old_host1": {"shard_name": "shard1"},
                    "new_host1": {"shard_name": "shard1"},
                    "old_host2": {"shard_name": "shard2"},
                    "new_host2": {"shard_name": "shard2"},
                },
                ["new_host1", "new_host2"],
                "shard2",
                [{"fqdn": "old_host2"}, {"fqdn": "new_host2"}],
            ],
            'result': ("old_host2", "shard2"),
        },
    ]

    for data in test_data:
        assert ClickHouseClusterModify._choose_backup_host(*data['args']) == data['result'], (
            'Unexpected backup host choice in test - ' + data['message']
        )
