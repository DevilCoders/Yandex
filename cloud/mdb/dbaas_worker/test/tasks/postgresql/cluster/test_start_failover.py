"""
PostgreSQL cluster start failover tests
"""

from test.mocks import checked_run_task_with_mocks
from test.tasks.postgresql.utils import get_postgresql_compute_host, get_postgresql_porto_host
from test.tasks.utils import check_mlock_usage, check_task_interrupt_consistency


def test_porto_postgresql_cluster_start_failover_interrupt_consistency(mocker):
    """
    Check porto start failover interruptions
    """
    args = {
        'hosts': {
            'host1': get_postgresql_porto_host(geo='geo1'),
            'host2': get_postgresql_porto_host(geo='geo2'),
            'host3': get_postgresql_porto_host(geo='geo3'),
        },
        'target_host': 'host1',
    }
    *_, state = checked_run_task_with_mocks(
        mocker, 'postgresql_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )

    args['zk_hosts'] = 'test-zk'
    state['zookeeper']['contenders'] = ['host3']
    state['zookeeper']['pgsync'] = {'cid-test': {'timeline': '1'}}

    check_task_interrupt_consistency(
        mocker,
        'postgresql_cluster_start_failover',
        args,
        state,
    )


def test_compute_postgresql_cluster_start_failover_interrupt_consistency(mocker):
    """
    Check compute start failover interruptions
    """
    args = {
        'hosts': {
            'host1': get_postgresql_compute_host(geo='geo1'),
            'host2': get_postgresql_compute_host(geo='geo2'),
            'host3': get_postgresql_compute_host(geo='geo3'),
        },
        # missing target host here is intended
    }
    *_, state = checked_run_task_with_mocks(
        mocker, 'postgresql_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )

    args['zk_hosts'] = 'test-zk'
    state['zookeeper']['contenders'] = ['host3']
    state['zookeeper']['pgsync'] = {'cid-test': {'timeline': '1'}}

    check_task_interrupt_consistency(
        mocker,
        'postgresql_cluster_start_failover',
        args,
        state,
    )


def test_postgresql_cluster_start_failover_mlock_usage(mocker):
    """
    Check start failover mlock usage
    """
    args = {
        'hosts': {
            'host1': get_postgresql_porto_host(geo='geo1'),
            'host2': get_postgresql_porto_host(geo='geo2'),
            'host3': get_postgresql_porto_host(geo='geo3'),
        },
        'target_host': 'host1',
    }
    *_, state = checked_run_task_with_mocks(
        mocker, 'postgresql_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )

    args['zk_hosts'] = 'test-zk'
    state['zookeeper']['contenders'] = ['host3']
    state['zookeeper']['pgsync'] = {'cid-test': {'timeline': '1'}}

    check_mlock_usage(
        mocker,
        'postgresql_cluster_start_failover',
        args,
        state,
    )
