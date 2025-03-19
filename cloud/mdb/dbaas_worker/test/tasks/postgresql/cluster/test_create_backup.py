"""
PostgreSQL cluster create backup tests
"""

from test.mocks import checked_run_task_with_mocks
from test.tasks.postgresql.utils import get_postgresql_compute_host, get_postgresql_porto_host
from test.tasks.utils import check_mlock_usage, check_rejected, check_task_interrupt_consistency


def test_porto_postgresql_cluster_create_backup_interrupt_consistency(mocker):
    """
    Check porto create backup interruptions
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

    args['zk_hosts'] = 'test-zk'
    state['zookeeper']['contenders'] = ['host3']

    check_task_interrupt_consistency(
        mocker,
        'postgresql_cluster_create_backup',
        args,
        state,
    )


def test_compute_postgresql_cluster_create_backup_interrupt_consistency(mocker):
    """
    Check compute create backup interruptions
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

    args['zk_hosts'] = 'test-zk'
    state['zookeeper']['contenders'] = ['host3']

    check_task_interrupt_consistency(
        mocker,
        'postgresql_cluster_create_backup',
        args,
        state,
    )


def test_postgresql_cluster_create_backup_mlock_usage(mocker):
    """
    Check create backup mlock usage
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

    args['zk_hosts'] = 'test-zk'
    state['zookeeper']['contenders'] = ['host3']

    check_mlock_usage(
        mocker,
        'postgresql_cluster_create_backup',
        args,
        state,
    )


def test_postgresql_cluster_create_backup_revertable(mocker):
    """
    Check create backup revertability
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

    args['zk_hosts'] = 'test-zk'
    state['zookeeper']['contenders'] = ['host3']
    state['fail_actions'].add('deploy-v2-shipment-get-host3-state.sls')

    check_rejected(
        mocker,
        'postgresql_cluster_create_backup',
        args,
        state,
    )
