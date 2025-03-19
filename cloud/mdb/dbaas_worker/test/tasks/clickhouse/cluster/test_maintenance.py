"""
ClickHouse cluster maintenance tests
"""

from test.mocks import checked_run_task_with_mocks
from test.tasks.clickhouse.utils import (
    get_clickhouse_compute_host,
    get_clickhouse_porto_host,
    get_zookeeper_compute_host,
    get_zookeeper_porto_host,
)
from test.tasks.utils import check_mlock_usage, check_task_interrupt_consistency, prepare_state


def test_porto_clickhouse_cluster_maintenance_interrupt_consistency(mocker):
    """
    Check porto maintenance interruptions
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

    check_task_interrupt_consistency(
        mocker,
        'clickhouse_cluster_maintenance',
        args,
        state,
    )


def test_compute_clickhouse_cluster_maintenance_interrupt_consistency(mocker):
    """
    Check compute maintenance interruptions
    """
    args = {
        'hosts': {
            'host1': get_clickhouse_compute_host(geo='geo1'),
            'host2': get_clickhouse_compute_host(geo='geo2'),
            'host3': get_zookeeper_compute_host(geo='geo1'),
            'host4': get_zookeeper_compute_host(geo='geo2'),
            'host5': get_zookeeper_compute_host(geo='geo3'),
        },
    }

    *_, state = checked_run_task_with_mocks(
        mocker, 'clickhouse_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )

    args['restart'] = True

    check_task_interrupt_consistency(
        mocker,
        'clickhouse_cluster_maintenance',
        args,
        state,
    )


def test_compute_offline_clickhouse_cluster_maintenance_interrupt_consistency(mocker):
    """
    Check compute maintenance interruptions
    """
    args = {
        'hosts': {
            'host1': get_clickhouse_compute_host(geo='geo1'),
            'host2': get_clickhouse_compute_host(geo='geo2'),
            'host3': get_zookeeper_compute_host(geo='geo1'),
            'host4': get_zookeeper_compute_host(geo='geo2'),
            'host5': get_zookeeper_compute_host(geo='geo3'),
        },
    }

    *_, state = checked_run_task_with_mocks(
        mocker, 'clickhouse_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )
    *_, state = checked_run_task_with_mocks(mocker, 'clickhouse_cluster_stop', args, state=state)

    args['restart'] = True

    check_task_interrupt_consistency(
        mocker,
        'clickhouse_cluster_maintenance',
        args,
        state,
        task_params={'cluster_status': 'MAINTAINING-OFFLINE'},
        ignore=[
            'instance.instance-1:stop initiated',
            'instance.instance-2:stop initiated',
            'instance.instance-3:stop initiated',
            'instance.instance-5:stop initiated',
            'instance.instance-4:stop initiated',
        ],
    )


def test_clickhouse_cluster_maintenance_mlock_usage(mocker):
    """
    Check maintenance mlock usage
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
        'clickhouse_cluster_maintenance',
        args,
        state,
    )


TLS_UPDATE_EXPECTED_ACTIONS = {
    'certificator-get-host1',
    'certificator-download-host1',
    'certificator-get-host2',
    'certificator-download-host2',
    'certificator-get-host3',
    'certificator-download-host3',
    'certificator-get-host4',
    'certificator-download-host4',
    'certificator-get-host5',
    'certificator-download-host5',
}


def test_clickhouse_cluster_offline_maintenance_tls_update(mocker):
    """
    Check tls always updated when offline maintenance
    """
    args = {
        'hosts': {
            'host1': get_clickhouse_compute_host(geo='geo1'),
            'host2': get_clickhouse_compute_host(geo='geo2'),
            'host3': get_zookeeper_compute_host(geo='geo1'),
            'host4': get_zookeeper_compute_host(geo='geo2'),
            'host5': get_zookeeper_compute_host(geo='geo3'),
        },
    }

    *_, state = checked_run_task_with_mocks(
        mocker, 'clickhouse_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )
    *_, state = checked_run_task_with_mocks(mocker, 'clickhouse_cluster_stop', args, state=state)
    initial_state = state

    args['restart'] = True

    start_state = prepare_state(initial_state)
    *_, final_state = checked_run_task_with_mocks(
        mocker,
        'clickhouse_cluster_maintenance',
        args,
        state=start_state,
        feature_flags=None,
        task_params={'cluster_status': 'MAINTAINING-OFFLINE'},
    )
    actions_contains_all_tls_actions = (
        set(final_state['actions']).intersection(TLS_UPDATE_EXPECTED_ACTIONS) == TLS_UPDATE_EXPECTED_ACTIONS
    )
    if not actions_contains_all_tls_actions:
        raise AssertionError('must update tls when offline maintenance')


def test_clickhouse_cluster_online_maintenance_tls_not_update_by_default(mocker):
    """
    Check tls not updated when online maintenance by default
    """
    args = {
        'hosts': {
            'host1': get_clickhouse_compute_host(geo='geo1'),
            'host2': get_clickhouse_compute_host(geo='geo2'),
            'host3': get_zookeeper_compute_host(geo='geo1'),
            'host4': get_zookeeper_compute_host(geo='geo2'),
            'host5': get_zookeeper_compute_host(geo='geo3'),
        },
    }

    *_, state = checked_run_task_with_mocks(
        mocker, 'clickhouse_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )
    *_, state = checked_run_task_with_mocks(mocker, 'clickhouse_cluster_stop', args, state=state)
    initial_state = state

    args['restart'] = True

    start_state = prepare_state(initial_state)
    *_, final_state = checked_run_task_with_mocks(
        mocker,
        'clickhouse_cluster_maintenance',
        args,
        state=start_state,
        feature_flags=None,
    )
    actions_contains_any_tls_actions = set(final_state['actions']).intersection(TLS_UPDATE_EXPECTED_ACTIONS)
    if actions_contains_any_tls_actions:
        raise AssertionError('must not update tls when online maintenance by default')


def test_clickhouse_cluster_online_maintenance_tls_update_if_explicit(mocker):
    """
    Check tls updated when online maintenance if explicit
    """
    args = {
        'hosts': {
            'host1': get_clickhouse_compute_host(geo='geo1'),
            'host2': get_clickhouse_compute_host(geo='geo2'),
            'host3': get_zookeeper_compute_host(geo='geo1'),
            'host4': get_zookeeper_compute_host(geo='geo2'),
            'host5': get_zookeeper_compute_host(geo='geo3'),
        },
    }

    *_, state = checked_run_task_with_mocks(
        mocker, 'clickhouse_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )
    *_, state = checked_run_task_with_mocks(mocker, 'clickhouse_cluster_stop', args, state=state)
    initial_state = state

    args['restart'] = True
    args['update_tls'] = True  # Explicitly set tls update

    start_state = prepare_state(initial_state)
    *_, final_state = checked_run_task_with_mocks(
        mocker,
        'clickhouse_cluster_maintenance',
        args,
        state=start_state,
        feature_flags=None,
    )
    actions_contains_all_tls_actions = (
        set(final_state['actions']).intersection(TLS_UPDATE_EXPECTED_ACTIONS) == TLS_UPDATE_EXPECTED_ACTIONS
    )
    if not actions_contains_all_tls_actions:
        raise AssertionError('must update tls when online maintenance if that explicitly set')
