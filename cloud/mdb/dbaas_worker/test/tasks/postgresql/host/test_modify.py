"""
PostgreSQL host modify tests
"""

from test.mocks import checked_run_task_with_mocks
from test.tasks.postgresql.utils import get_postgresql_compute_host, get_postgresql_porto_host
from test.tasks.utils import check_mlock_usage, check_task_interrupt_consistency


def test_porto_postgresql_host_modify_interrupt_consistency(mocker):
    """
    Check porto host modify interruptions
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
    state['zookeeper']['pgsync'] = {'cid-test': {'timeline': '1'}}

    args['host'] = {'fqdn': 'host3'}
    args['pillar'] = {'data': {'pgsync': {'replication_source': 'host1'}}}

    check_task_interrupt_consistency(
        mocker,
        'postgresql_host_modify',
        args,
        state,
    )


def test_compute_postgresql_host_modify_interrupt_consistency(mocker):
    """
    Check compute host modify interruptions
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
    state['zookeeper']['pgsync'] = {'cid-test': {'timeline': '1'}}

    args['host'] = {'fqdn': 'host3'}
    args['pillar'] = {'data': {'pgsync': {'replication_source': 'host1'}}}

    check_task_interrupt_consistency(
        mocker,
        'postgresql_host_modify',
        args,
        state,
    )


def test_postgresql_host_modify_mlock_usage(mocker):
    """
    Check host modify mlock usage
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
    state['zookeeper']['pgsync'] = {'cid-test': {'timeline': '1'}}

    args['host'] = {'fqdn': 'host3'}
    args['pillar'] = {'data': {'pgsync': {'replication_source': 'host1'}}}

    check_mlock_usage(
        mocker,
        'postgresql_host_modify',
        args,
        state,
    )
