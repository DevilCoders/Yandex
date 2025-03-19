"""
ClickHouse cluster create tests
"""

from test.mocks import get_state
from test.tasks.clickhouse.utils import (
    get_clickhouse_compute_host,
    get_clickhouse_porto_host,
    get_zookeeper_compute_host,
    get_zookeeper_porto_host,
)
from test.tasks.utils import check_mlock_usage, check_task_interrupt_consistency, check_alerts_synchronised


def test_porto_clickhouse_cluster_create_interrupt_consistency(mocker):
    """
    Check porto create interruptions
    """
    check_task_interrupt_consistency(
        mocker,
        'clickhouse_cluster_create',
        {
            'hosts': {
                'host1': get_clickhouse_porto_host(geo='geo1'),
                'host2': get_clickhouse_porto_host(geo='geo2'),
                'host3': get_zookeeper_porto_host(geo='geo1'),
                'host4': get_zookeeper_porto_host(geo='geo2'),
                'host5': get_zookeeper_porto_host(geo='geo3'),
            },
            's3_bucket': 'test-s3-bucket',
        },
        get_state(),
    )


def test_compute_clickhouse_cluster_create_interrupt_consistency(mocker):
    """
    Check compute create interruptions
    """
    check_task_interrupt_consistency(
        mocker,
        'clickhouse_cluster_create',
        {
            'hosts': {
                'host1': get_clickhouse_compute_host(geo='geo1'),
                'host2': get_clickhouse_compute_host(geo='geo2'),
                'host3': get_zookeeper_compute_host(geo='geo1'),
                'host4': get_zookeeper_compute_host(geo='geo2'),
                'host5': get_zookeeper_compute_host(geo='geo3'),
            },
            's3_buckets': {'backup': 'test-s3-bucket', 'cloud_storage': 'cloud-storage-bucket'},
        },
        get_state(),
    )


def test_clickhouse_cluster_create_mlock_usage(mocker):
    """
    Check create mlock usage
    """
    check_mlock_usage(
        mocker,
        'clickhouse_cluster_create',
        {
            'hosts': {
                'host1': get_clickhouse_porto_host(geo='geo1'),
                'host2': get_clickhouse_porto_host(geo='geo2'),
                'host3': get_zookeeper_porto_host(geo='geo1'),
                'host4': get_zookeeper_porto_host(geo='geo2'),
                'host5': get_zookeeper_porto_host(geo='geo3'),
            },
            's3_bucket': 'test-s3-bucket',
        },
        get_state(),
    )


def test_clickhouse_cluster_create_alert_sync(mocker):
    """
    Check create mlock usage
    """
    check_alerts_synchronised(
        mocker,
        'clickhouse_cluster_create',
        {
            'hosts': {
                'host1': get_clickhouse_porto_host(geo='geo1'),
                'host2': get_clickhouse_porto_host(geo='geo2'),
                'host3': get_zookeeper_porto_host(geo='geo1'),
                'host4': get_zookeeper_porto_host(geo='geo2'),
                'host5': get_zookeeper_porto_host(geo='geo3'),
            },
            's3_bucket': 'test-s3-bucket',
        },
        get_state(),
    )
