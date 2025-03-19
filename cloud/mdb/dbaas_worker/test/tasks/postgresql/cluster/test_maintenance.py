"""
PostgreSQL cluster maintenance executor tests
"""

from test.mocks import checked_run_task_with_mocks
from test.tasks.postgresql.utils import get_postgresql_compute_host, get_postgresql_porto_host
from test.tasks.utils import check_mlock_usage, check_task_interrupt_consistency

ZK_HOSTS = 'test-zk'


def test_porto_postgresql_cluster_test_maintenance_interrupt_consistency(mocker):
    """
    Check porto maintenance task interruptions
    """
    args = {
        'hosts': {
            'host1': get_postgresql_porto_host(geo='geo1'),
            'host2': get_postgresql_porto_host(geo='geo2'),
            'host3': get_postgresql_porto_host(geo='geo3'),
        },
    }
    *_, state = checked_run_task_with_mocks(
        mocker, 'postgresql_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )

    args.update(
        {
            'restart': True,
            'update_tls': True,
            'zk_hosts': ZK_HOSTS,
        }
    )

    state['zookeeper']['contenders'] = ['host3']

    check_task_interrupt_consistency(
        mocker,
        'postgresql_cluster_maintenance',
        args,
        state,
    )


def test_porto_postgresql_cluster_test_maintenance_with_force_tls_and_random_order_interrupt_consistency(mocker):
    """
    Check porto maintenance task interruptions
    """
    args = {
        'hosts': {
            'host1': get_postgresql_porto_host(geo='geo1'),
            'host2': get_postgresql_porto_host(geo='geo2'),
            'host3': get_postgresql_porto_host(geo='geo3'),
        },
    }
    *_, state = checked_run_task_with_mocks(
        mocker, 'postgresql_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )

    args.update(
        {
            'force_tls_certs': True,
            'random_order': True,
            'update_tls': True,
            'zk_hosts': ZK_HOSTS,
        }
    )

    state['zookeeper']['contenders'] = ['host3']

    check_task_interrupt_consistency(
        mocker,
        'postgresql_cluster_maintenance',
        args,
        state,
    )


def test_compute_postgresql_cluster_maintenance_interrupt_consistency(mocker):
    """
    Check compute maintenance task interruptions
    """
    args = {
        'hosts': {
            'host1': get_postgresql_compute_host(geo='geo1'),
            'host2': get_postgresql_compute_host(geo='geo2'),
            'host3': get_postgresql_compute_host(geo='geo3'),
        },
    }
    *_, state = checked_run_task_with_mocks(
        mocker, 'postgresql_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )

    args.update(
        {
            'restart': True,
            'update_tls': True,
            'zk_hosts': ZK_HOSTS,
        }
    )

    state['zookeeper']['contenders'] = ['host3']

    check_task_interrupt_consistency(
        mocker,
        'postgresql_cluster_maintenance',
        args,
        state,
    )


def test_compute_offline_postgresql_cluster_maintenance_interrupt_consistency(mocker):
    """
    Check compute stopped cluster maintenance interruptions
    """
    args = {
        'hosts': {
            'host1': get_postgresql_compute_host(geo='geo1'),
            'host2': get_postgresql_compute_host(geo='geo2'),
            'host3': get_postgresql_compute_host(geo='geo3'),
        },
    }
    *_, state = checked_run_task_with_mocks(
        mocker, 'postgresql_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )
    *_, state = checked_run_task_with_mocks(mocker, 'postgresql_cluster_stop', args, state=state)

    args.update(
        {
            'restart': True,
            'update_tls': True,
            'zk_hosts': ZK_HOSTS,
        }
    )

    state['zookeeper']['contenders'] = ['host3']

    check_task_interrupt_consistency(
        mocker,
        'postgresql_cluster_maintenance',
        args,
        state,
        task_params={'cluster_status': 'STOPPED'},
        ignore=[
            'instance.instance-1:stop initiated',
            'instance.instance-2:stop initiated',
            'instance.instance-3:stop initiated',
        ],
    )


def test_postgresql_cluster_test_maintenance_mlock_usage(mocker):
    """
    Check maintenance task mlock usage
    """
    args = {
        'hosts': {
            'host1': get_postgresql_porto_host(geo='geo1'),
            'host2': get_postgresql_porto_host(geo='geo2'),
            'host3': get_postgresql_porto_host(geo='geo3'),
        },
    }
    *_, state = checked_run_task_with_mocks(
        mocker, 'postgresql_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )

    args.update(
        {
            'restart': True,
            'update_tls': True,
            'zk_hosts': ZK_HOSTS,
        }
    )

    state['zookeeper']['contenders'] = ['host3']

    check_mlock_usage(
        mocker,
        'postgresql_cluster_maintenance',
        args,
        state,
    )
