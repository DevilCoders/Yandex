"""
Greenplum cluster modify tests
"""

import json
from operator import itemgetter

from hamcrest import assert_that, has_entries, has_items

from test.mocks import checked_run_task_with_mocks
from test.tasks.greenplum.utils import (
    get_greenplum_master_compute_host,
    get_greenplum_segment_compute_host,
    get_greenplum_master_porto_host,
    get_greenplum_segment_porto_host,
)
from test.tasks.utils import (
    check_mlock_usage,
    check_task_interrupt_consistency,
    check_rejected,
    check_alerts_synchronised,
)


def test_greenplum_cluster_modify_not_healthy(mocker):
    """
    Check modify not healthy cluster
    """
    args = {
        'hosts': {
            'host1': get_greenplum_master_porto_host(geo='geo1'),
            'host2': get_greenplum_master_porto_host(geo='geo1'),
            'host3': get_greenplum_segment_porto_host(geo='geo1'),
            'host4': get_greenplum_segment_porto_host(geo='geo1'),
            'host5': get_greenplum_segment_porto_host(geo='geo1'),
        },
    }

    *_, state = checked_run_task_with_mocks(
        mocker, 'greenplum_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )

    args['restart'] = True

    state['juggler']['host1']['status'] = 'CRIT'

    check_rejected(
        mocker,
        'greenplum_cluster_modify',
        args,
        state,
    )


def test_porto_greenplum_cluster_modify_interrupt_consistency(mocker):
    """
    Check porto modify interruptions
    """

    args = {
        'hosts': {
            'host1': get_greenplum_master_porto_host(geo='geo1'),
            'host2': get_greenplum_master_porto_host(geo='geo1'),
            'host3': get_greenplum_segment_porto_host(geo='geo1'),
            'host4': get_greenplum_segment_porto_host(geo='geo1'),
            'host5': get_greenplum_segment_porto_host(geo='geo1'),
        },
    }

    *_, state = checked_run_task_with_mocks(
        mocker, 'greenplum_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )

    args['restart'] = True

    check_task_interrupt_consistency(
        mocker,
        'greenplum_cluster_modify',
        args,
        state,
        ignore=[],
    )


def test_compute_greenplum_cluster_modify_interrupt_consistency(mocker):
    """
    Check compute modify interruptions
    """
    args = {
        'hosts': {
            'host1': get_greenplum_master_compute_host(geo='geo1'),
            'host2': get_greenplum_master_compute_host(geo='geo1'),
            'host3': get_greenplum_segment_compute_host(geo='geo1'),
            'host4': get_greenplum_segment_compute_host(geo='geo1'),
            'host5': get_greenplum_segment_compute_host(geo='geo1'),
        },
    }

    *_, state = checked_run_task_with_mocks(
        mocker, 'greenplum_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )

    interrupted_state = check_task_interrupt_consistency(
        mocker,
        'greenplum_cluster_modify',
        args,
        state,
        ignore=[],
    )
    for hostname in ['host1', 'host2', 'host3', 'host4', 'host5']:
        assert_that(
            interrupted_state['compute']['instances'].values(),
            has_items(has_entries({'fqdn': hostname, 'status': 'RUNNING'})),
        )


def test_greenplum_cluster_modify_mlock_usage(mocker):
    """
    Check modify mlock usage
    """
    args = {
        'hosts': {
            'host1': get_greenplum_master_compute_host(geo='geo1'),
            'host2': get_greenplum_master_compute_host(geo='geo1'),
            'host3': get_greenplum_segment_compute_host(geo='geo1'),
            'host4': get_greenplum_segment_compute_host(geo='geo1'),
            'host5': get_greenplum_segment_compute_host(geo='geo1'),
        },
    }

    *_, state = checked_run_task_with_mocks(
        mocker, 'greenplum_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )

    check_mlock_usage(
        mocker,
        'greenplum_cluster_modify',
        args,
        state,
    )


def test_greenplum_cluster_modify_alert_sync(mocker):
    """
    Check modify mlock usage
    """
    args = {
        'hosts': {
            'host1': get_greenplum_master_compute_host(geo='geo1'),
            'host2': get_greenplum_master_compute_host(geo='geo1'),
            'host3': get_greenplum_segment_compute_host(geo='geo1'),
            'host4': get_greenplum_segment_compute_host(geo='geo1'),
            'host5': get_greenplum_segment_compute_host(geo='geo1'),
        },
    }

    *_, state = checked_run_task_with_mocks(
        mocker, 'greenplum_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )

    args['restart'] = True

    check_alerts_synchronised(
        mocker,
        'greenplum_cluster_modify',
        args,
        state,
    )


def test_greenplum_cluster_modify_alt_conductor_group(mocker):
    """
    Check that cluster correctly changes alt conductor group on modify
    """
    args = {
        'hosts': {
            'host1': get_greenplum_master_compute_host(geo='geo1', flavor='alt-test-flavor', subcid='m-subcid'),
            'host2': get_greenplum_master_compute_host(geo='geo1', flavor='alt-test-flavor', subcid='m-subcid'),
            'host3': get_greenplum_segment_compute_host(geo='geo1', flavor='alt-test-flavor', subcid='s-subcid'),
            'host4': get_greenplum_segment_compute_host(geo='geo1', flavor='alt-test-flavor', subcid='s-subcid'),
            'host5': get_greenplum_segment_compute_host(geo='geo1', flavor='alt-test-flavor', subcid='s-subcid'),
        },
    }
    *_, state = checked_run_task_with_mocks(
        mocker, 'greenplum_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )

    assert (
        state['conductor']['groups']['db_m_subcid']['parent_ids'][0]
        == state['conductor']['groups']['mdb_test_altgroup']['id']
    ), 'Unexpected masternode initial group id'

    assert (
        state['conductor']['groups']['db_s_subcid']['parent_ids'][0]
        == state['conductor']['groups']['mdb_test_altgroup']['id']
    ), 'Unexpected segmentnode initial group id'

    for host in args['hosts'].copy():
        args['hosts'][host]['flavor'] = 'test-flavor'

    args['restart'] = True

    *_, state = checked_run_task_with_mocks(mocker, 'greenplum_cluster_modify', args, state=state)

    assert (
        state['conductor']['groups']['db_m_subcid']['parent_ids'][0]
        == state['conductor']['groups']['mdb_test_greenplum_master']['id']
    ), 'Unexpected masternode group id after modify'

    assert (
        state['conductor']['groups']['db_s_subcid']['parent_ids'][0]
        == state['conductor']['groups']['mdb_test_greenplum_segment']['id']
    ), 'Unexpected segmentnode group id after modify'


def _find_shipments_from_state(state, host, state_type, operation):
    """
    Finds all shipments that match given host, state type and operation
    """
    matching_shipments = []
    for shipment in state['deploy-v2']['shipments'].values():
        if (
            shipment['id'].startswith(f'{host}-{state_type}')
            and shipment['commands'][0]['type'] == state_type
            and shipment['commands'][0]['arguments'][0] == operation
        ):
            matching_shipments.append(shipment)

    return matching_shipments


def _get_number_from_shipment(shipment):
    """
    Extracts instance number from shipment
    """
    try:
        return int(shipment['id'].split('-')[-1])
    except ValueError:
        return 1


def _get_pillar_from_shipment(shipment):
    """
    Extracts pillar from shipment's first command
    """
    for arg in shipment['commands'][0]['arguments']:
        if arg.startswith('pillar='):
            pillar = json.loads(arg[7:])
            return pillar
    return {}


def test_greenplum_cluster_modify_change_password_to_pillar(mocker):
    """
    Check that cluster modify task passes sync-passwords argument to pillar
    in components.dbaas-operations.service shipments
    """
    args = {
        'hosts': {
            'host1': get_greenplum_master_compute_host(geo='geo1'),
            'host2': get_greenplum_master_compute_host(geo='geo1'),
            'host3': get_greenplum_segment_compute_host(geo='geo1'),
            'host4': get_greenplum_segment_compute_host(geo='geo1'),
            'host5': get_greenplum_segment_compute_host(geo='geo1'),
        },
    }
    *_, state = checked_run_task_with_mocks(
        mocker, 'greenplum_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )

    state['deploy-v2']['record_all_shipments'] = True
    *_, state_no_password_change = checked_run_task_with_mocks(mocker, 'greenplum_cluster_modify', args, state=state)

    shipments = _find_shipments_from_state(
        state_no_password_change, 'host1', 'state.sls', 'components.dbaas-operations.service'
    )
    assert all(
        _get_pillar_from_shipment(shipment)['sync-passwords'] is False for shipment in shipments
    ), 'Not all components.dbaas-operations.service shipments had sync-passwords=False in pillar'

    last_shipment_num = _get_number_from_shipment(sorted(shipments, key=itemgetter('id'), reverse=True)[0])

    args['sync-passwords'] = True
    *_, state_with_password_change = checked_run_task_with_mocks(mocker, 'greenplum_cluster_modify', args, state=state)

    shipments = [
        shipment
        for shipment in _find_shipments_from_state(
            state_with_password_change, 'host1', 'state.sls', 'components.dbaas-operations.service'
        )
        if _get_number_from_shipment(shipment) > last_shipment_num
    ]
    assert all(
        _get_pillar_from_shipment(shipment)['sync-passwords'] is True for shipment in shipments
    ), 'Not all components.dbaas-operations.service shipments had sync-passwords=True in pillar'
