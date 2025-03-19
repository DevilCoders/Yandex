"""
ClickHouse cluster add zookeeper tests
"""

from test.mocks import checked_run_task_with_mocks
from test.tasks.clickhouse.utils import (
    get_clickhouse_compute_host,
    get_clickhouse_porto_host,
    get_zookeeper_compute_host,
    get_zookeeper_porto_host,
)
from test.tasks.utils import check_mlock_usage, check_task_interrupt_consistency


def test_porto_clickhouse_cluster_add_zookeeper_interrupt_consistency(mocker):
    """
    Check porto add zookeeper interruptions
    """
    args = {
        'hosts': {
            'host1': get_clickhouse_porto_host(geo='geo1'),
            'host2': get_clickhouse_porto_host(geo='geo2'),
        },
    }

    *_, state = checked_run_task_with_mocks(
        mocker, 'clickhouse_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )

    args['hosts']['host3'] = get_zookeeper_porto_host(geo='geo1')
    args['hosts']['host4'] = get_zookeeper_porto_host(geo='geo2')
    args['hosts']['host5'] = get_zookeeper_porto_host(geo='geo3')

    check_task_interrupt_consistency(
        mocker,
        'clickhouse_add_zookeeper',
        args,
        state,
    )


def test_compute_clickhouse_cluster_add_zookeeper_interrupt_consistency(mocker):
    """
    Check compute add zookeeper interruptions
    """
    args = {
        'hosts': {
            'host1': get_clickhouse_compute_host(geo='geo1'),
            'host2': get_clickhouse_compute_host(geo='geo2'),
        },
    }

    *_, state = checked_run_task_with_mocks(
        mocker, 'clickhouse_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )

    args['hosts']['host3'] = get_zookeeper_compute_host(geo='geo1')
    args['hosts']['host4'] = get_zookeeper_compute_host(geo='geo2')
    args['hosts']['host5'] = get_zookeeper_compute_host(geo='geo3')

    check_task_interrupt_consistency(
        mocker,
        'clickhouse_add_zookeeper',
        args,
        state,
    )


def test_clickhouse_cluster_add_zookeeper_alert_sync(mocker):
    """
    Check add zookeeper mlock usage
    """
    args = {
        'hosts': {
            'host1': get_clickhouse_porto_host(geo='geo1'),
            'host2': get_clickhouse_porto_host(geo='geo2'),
        },
    }

    *_, state = checked_run_task_with_mocks(
        mocker, 'clickhouse_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )

    args['hosts']['host3'] = get_zookeeper_porto_host(geo='geo1')
    args['hosts']['host4'] = get_zookeeper_porto_host(geo='geo2')
    args['hosts']['host5'] = get_zookeeper_porto_host(geo='geo3')

    check_mlock_usage(
        mocker,
        'clickhouse_add_zookeeper',
        args,
        state,
    )
