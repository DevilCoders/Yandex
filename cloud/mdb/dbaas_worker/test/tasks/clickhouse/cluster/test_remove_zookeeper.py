"""
ClickHouse cluster add zookeeper tests
"""

from test.mocks import checked_run_task_with_mocks
from test.tasks.clickhouse.utils import (
    get_clickhouse_porto_host,
    get_zookeeper_porto_host,
)
from test.tasks.utils import check_mlock_usage, check_task_interrupt_consistency


def test_porto_clickhouse_cluster_remove_zookeeper_interrupt_consistency(mocker):
    """
    Check porto add zookeeper interruptions
    """
    ch1 = get_clickhouse_porto_host(geo='geo1')
    ch2 = get_clickhouse_porto_host(geo='geo2')
    zk1 = get_zookeeper_porto_host(geo='geo1')
    zk2 = get_zookeeper_porto_host(geo='geo2')
    zk3 = get_zookeeper_porto_host(geo='geo3')
    create_args = {
        'hosts': {
            'host1': ch1,
            'host2': ch2,
            'zk1': zk1,
            'zk2': zk2,
            'zk3': zk3,
        },
    }

    *_, state = checked_run_task_with_mocks(
        mocker, 'clickhouse_cluster_create', dict(**create_args, s3_bucket='test-s3-bucket')
    )

    delete_zk_args = {
        'hosts': {'host1': ch1, 'host2': ch2},
        'delete_hosts': [_with_fqdn(zk1, 'zk1'), _with_fqdn(zk2, 'zk2'), _with_fqdn(zk3, 'zk3')],
    }
    check_task_interrupt_consistency(
        mocker,
        'clickhouse_remove_zookeeper',
        delete_zk_args,
        state,
    )


def test_clickhouse_cluster_remove_zookeeper_mlock_usage(mocker):
    """
    Check add zookeeper mlock usage
    """
    ch1 = get_clickhouse_porto_host(geo='geo1')
    ch2 = get_clickhouse_porto_host(geo='geo2')
    zk1 = get_zookeeper_porto_host(geo='geo1')
    zk2 = get_zookeeper_porto_host(geo='geo2')
    zk3 = get_zookeeper_porto_host(geo='geo3')
    create_args = {
        'hosts': {
            'host1': ch1,
            'host2': ch2,
            'zk1': zk1,
            'zk2': zk2,
            'zk3': zk3,
        },
    }

    *_, state = checked_run_task_with_mocks(
        mocker, 'clickhouse_cluster_create', dict(**create_args, s3_bucket='test-s3-bucket')
    )

    delete_zk_args = {
        'hosts': {'host1': ch1, 'host2': ch2},
        'delete_hosts': [_with_fqdn(zk1, 'zk1'), _with_fqdn(zk2, 'zk2'), _with_fqdn(zk3, 'zk3')],
    }
    check_mlock_usage(
        mocker,
        'clickhouse_remove_zookeeper',
        delete_zk_args,
        state,
    )


def _with_fqdn(host, fqdn):
    host.update(dict(fqdn=fqdn))
    return host
