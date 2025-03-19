"""
PostgreSQL cluster fast maintenance executor tests
"""

from test.mocks import checked_run_task_with_mocks
from test.tasks.postgresql.utils import get_postgresql_compute_host, get_postgresql_porto_host
from test.tasks.utils import check_mlock_usage, check_task_interrupt_consistency

ZK_HOSTS = 'test-zk'


def test_porto_postgresql_cluster_test_fast_maintenance_interrupt_consistency(mocker):
    """
    Check porto fast maintenance task interruptions
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
        'postgresql_cluster_fast_maintenance',
        args,
        state,
    )


def test_porto_postgresql_cluster_test_fast_maintenance_with_force_tls_and_random_order_interrupt_consistency(mocker):
    """
    Check porto fast maintenance task interruptions
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
        'postgresql_cluster_fast_maintenance',
        args,
        state,
    )


def test_compute_postgresql_cluster_fast_maintenance_interrupt_consistency(mocker):
    """
    Check compute fast maintenance task interruptions
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
        'postgresql_cluster_fast_maintenance',
        args,
        state,
    )


def test_postgresql_cluster_test_fast_maintenance_mlock_usage(mocker):
    """
    Check fast maintenance task mlock usage
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
        'postgresql_cluster_fast_maintenance',
        args,
        state,
    )
