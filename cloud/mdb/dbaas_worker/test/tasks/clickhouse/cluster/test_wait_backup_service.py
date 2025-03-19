"""
Clickhouse cluster wait_backup_service tests
"""

from test.mocks import checked_run_task_with_mocks
from test.tasks.clickhouse.utils import (
    get_clickhouse_compute_host,
    get_clickhouse_porto_host,
    get_zookeeper_compute_host,
    get_zookeeper_porto_host,
)
from test.tasks.utils import check_mlock_usage, check_task_interrupt_consistency


def test_porto_clickhouse_cluster_test_wait_backup_service_interrupt_consistency(mocker):
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

    args.update(
        {
            'backup_ids': [1],
        }
    )

    check_task_interrupt_consistency(
        mocker,
        'clickhouse_cluster_wait_backup_service',
        args,
        state,
    )


def test_compute_clickhouse_cluster_wait_backup_service_interrupt_consistency(mocker):
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

    args.update(
        {
            'backup_ids': [1],
        }
    )

    check_task_interrupt_consistency(
        mocker,
        'clickhouse_cluster_wait_backup_service',
        args,
        state,
    )


def test_clickhouse_cluster_test_wait_backup_service_mlock_usage(mocker):
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

    args.update(
        {
            'backup_ids': [1],
        }
    )

    check_mlock_usage(
        mocker,
        'clickhouse_cluster_wait_backup_service',
        args,
        state,
    )
