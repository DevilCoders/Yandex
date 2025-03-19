"""
Opensearch cluster modify tests
"""

from hamcrest import assert_that, has_entries, has_items

from test.mocks import checked_run_task_with_mocks
from test.tasks.opensearch.utils import (
    get_opensearch_datanode_compute_host,
    get_opensearch_datanode_porto_host,
    get_opensearch_masternode_compute_host,
    get_opensearch_masternode_porto_host,
)
from test.tasks.utils import (
    check_mlock_usage,
    check_task_interrupt_consistency,
    check_rejected,
    check_alerts_synchronised,
)


def test_opensearch_cluster_modify_data_only(mocker):
    """
    Check successfully modify with data only nodes
    """
    args = {
        'hosts': {
            'host1': get_opensearch_datanode_porto_host(geo='geo1'),
            'host2': get_opensearch_datanode_porto_host(geo='geo2'),
            'host3': get_opensearch_datanode_porto_host(geo='geo3'),
        },
    }

    *_, state = checked_run_task_with_mocks(
        mocker, 'opensearch_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )

    args['restart'] = True

    checked_run_task_with_mocks(
        mocker,
        'opensearch_cluster_modify',
        args,
        state=state,
    )


def test_opensearch_cluster_modify_no_restart(mocker):
    """
    Check successfully modify with data only nodes
    """
    args = {
        'hosts': {
            'host1': get_opensearch_datanode_porto_host(geo='geo1'),
            'host2': get_opensearch_datanode_porto_host(geo='geo2'),
            'host3': get_opensearch_datanode_porto_host(geo='geo3'),
        },
    }

    *_, state = checked_run_task_with_mocks(
        mocker, 'opensearch_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )

    checked_run_task_with_mocks(
        mocker,
        'opensearch_cluster_modify',
        args,
        state=state,
    )


def test_opensearch_cluster_modify_not_healthy(mocker):
    """
    Check modify not healthy cluster
    """
    args = {
        'hosts': {
            'host1': get_opensearch_datanode_porto_host(geo='geo1'),
            'host2': get_opensearch_datanode_porto_host(geo='geo2'),
            'host3': get_opensearch_masternode_porto_host(geo='geo1'),
            'host4': get_opensearch_masternode_porto_host(geo='geo2'),
            'host5': get_opensearch_masternode_porto_host(geo='geo3'),
        },
    }

    *_, state = checked_run_task_with_mocks(
        mocker, 'opensearch_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )

    args['restart'] = True

    state['juggler']['host1']['status'] = 'CRIT'

    check_rejected(
        mocker,
        'opensearch_cluster_modify',
        args,
        state,
    )


def test_porto_opensearch_cluster_modify_interrupt_consistency(mocker):
    """
    Check porto modify interruptions
    """
    args = {
        'hosts': {
            'host1': get_opensearch_datanode_porto_host(geo='geo1'),
            'host2': get_opensearch_datanode_porto_host(geo='geo2'),
            'host3': get_opensearch_masternode_porto_host(geo='geo1'),
            'host4': get_opensearch_masternode_porto_host(geo='geo2'),
            'host5': get_opensearch_masternode_porto_host(geo='geo3'),
        },
    }

    *_, state = checked_run_task_with_mocks(
        mocker, 'opensearch_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
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
        'opensearch_cluster_modify',
        args,
        state,
        # we skip pre-restart on already moving container
        ignore=['deployv2.components.dbaas-operations.run-pre-restart-script.host1.host1-state.sls:started'],
    )


def test_compute_opensearch_cluster_modify_interrupt_consistency(mocker):
    """
    Check compute modify interruptions
    """
    args = {
        'hosts': {
            'host1': get_opensearch_datanode_compute_host(
                geo='geo1', disk_type_id='local-ssd', space_limit=107374182400
            ),
            'host2': get_opensearch_datanode_compute_host(geo='geo2'),
            'host3': get_opensearch_masternode_compute_host(geo='geo1'),
            'host4': get_opensearch_masternode_compute_host(geo='geo2'),
            'host5': get_opensearch_masternode_compute_host(geo='geo3'),
        },
    }

    *_, state = checked_run_task_with_mocks(
        mocker, 'opensearch_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )

    # to cause local disk resize
    args['hosts']['host1']['space_limit'] = args['hosts']['host1']['space_limit'] * 2

    # to cause network disk resize and resources change
    args['hosts']['host3']['cpu_limit'] = args['hosts']['host2']['cpu_limit'] * 2
    args['hosts']['host3']['space_limit'] = args['hosts']['host2']['space_limit'] * 2

    # to cause network disk downscale
    args['hosts']['host4']['space_limit'] = args['hosts']['host3']['space_limit'] // 2

    args['restart'] = True

    interrupted_state = check_task_interrupt_consistency(
        mocker,
        'opensearch_cluster_modify',
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


def test_opensearch_cluster_modify_mlock_usage(mocker):
    """
    Check modify mlock usage
    """
    args = {
        'hosts': {
            'host1': get_opensearch_datanode_porto_host(geo='geo1'),
            'host2': get_opensearch_datanode_porto_host(geo='geo2'),
            'host3': get_opensearch_masternode_porto_host(geo='geo1'),
            'host4': get_opensearch_masternode_porto_host(geo='geo2'),
            'host5': get_opensearch_masternode_porto_host(geo='geo3'),
        },
    }

    *_, state = checked_run_task_with_mocks(
        mocker, 'opensearch_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )

    args['restart'] = True

    check_mlock_usage(
        mocker,
        'opensearch_cluster_modify',
        args,
        state,
    )


def test_opensearch_cluster_modify_alert_sync(mocker):
    """
    Check modify mlock usage
    """
    args = {
        'hosts': {
            'host1': get_opensearch_datanode_porto_host(geo='geo1'),
            'host2': get_opensearch_datanode_porto_host(geo='geo2'),
            'host3': get_opensearch_masternode_porto_host(geo='geo1'),
            'host4': get_opensearch_masternode_porto_host(geo='geo2'),
            'host5': get_opensearch_masternode_porto_host(geo='geo3'),
        },
    }

    *_, state = checked_run_task_with_mocks(
        mocker, 'opensearch_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )

    args['restart'] = True

    check_alerts_synchronised(
        mocker,
        'opensearch_cluster_modify',
        args,
        state,
    )


def test_opensearch_cluster_modify_alt_conductor_group(mocker):
    """
    Check that cluster correctly changes alt conductor group on modify
    """
    args = {
        'hosts': {
            'host1': get_opensearch_datanode_porto_host(geo='geo1', flavor='alt-test-flavor', subcid='dn-subcid'),
            'host2': get_opensearch_datanode_porto_host(geo='geo2', flavor='alt-test-flavor', subcid='dn-subcid'),
            'host3': get_opensearch_masternode_porto_host(geo='geo1', flavor='alt-test-flavor', subcid='mn-subcid'),
            'host4': get_opensearch_masternode_porto_host(geo='geo2', flavor='alt-test-flavor', subcid='mn-subcid'),
            'host5': get_opensearch_masternode_porto_host(geo='geo3', flavor='alt-test-flavor', subcid='mn-subcid'),
        },
    }
    *_, state = checked_run_task_with_mocks(
        mocker, 'opensearch_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )

    assert (
        state['conductor']['groups']['db_dn_subcid']['parent_ids'][0]
        == state['conductor']['groups']['mdb_test_altgroup']['id']
    ), 'Unexpected datanode initial group id'

    assert (
        state['conductor']['groups']['db_mn_subcid']['parent_ids'][0]
        == state['conductor']['groups']['mdb_test_altgroup']['id']
    ), 'Unexpected masternode initial group id'

    for host in args['hosts'].copy():
        args['hosts'][host]['flavor'] = 'test-flavor'

    args['restart'] = True

    *_, state = checked_run_task_with_mocks(mocker, 'opensearch_cluster_modify', args, state=state)

    assert (
        state['conductor']['groups']['db_dn_subcid']['parent_ids'][0]
        == state['conductor']['groups']['mdb_test_opensearch_data']['id']
    ), 'Unexpected datanode group id after modify'

    assert (
        state['conductor']['groups']['db_mn_subcid']['parent_ids'][0]
        == state['conductor']['groups']['mdb_test_opensearch_master']['id']
    ), 'Unexpected masternode group id after modify'
