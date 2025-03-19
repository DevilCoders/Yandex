"""
Kafka cluster create tests
"""

from test.mocks import get_state
from test.tasks.kafka.utils import (
    get_kafka_compute_host,
    get_kafka_porto_host,
    get_zookeeper_compute_host,
    get_zookeeper_porto_host,
)
from test.tasks.utils import check_mlock_usage, check_task_interrupt_consistency, check_alerts_synchronised


def test_porto_kafka_cluster_create_interrupt_consistency(mocker):
    """
    Check porto create interruptions
    """
    check_task_interrupt_consistency(
        mocker,
        'kafka_cluster_create',
        {
            'hosts': {
                'host1': get_kafka_porto_host(geo='geo1'),
                'host2': get_kafka_porto_host(geo='geo2'),
                'host3': get_kafka_porto_host(geo='geo3'),
                'host4': get_zookeeper_porto_host(geo='geo1'),
                'host5': get_zookeeper_porto_host(geo='geo2'),
                'host6': get_zookeeper_porto_host(geo='geo3'),
            },
            's3_bucket': 'test-s3-bucket',
        },
        get_state(),
    )


def test_compute_kafka_cluster_create_interrupt_consistency(mocker):
    """
    Check compute create interruptions
    """
    check_task_interrupt_consistency(
        mocker,
        'kafka_cluster_create',
        {
            'hosts': {
                'host1': get_kafka_compute_host(geo='geo1'),
                'host2': get_kafka_compute_host(geo='geo2'),
                'host3': get_kafka_compute_host(geo='geo3'),
                'host4': get_zookeeper_compute_host(geo='geo1'),
                'host5': get_zookeeper_compute_host(geo='geo2'),
                'host6': get_zookeeper_compute_host(geo='geo3'),
            },
            's3_bucket': 'test-s3-bucket',
        },
        get_state(),
    )


def test_kafka_cluster_create_mlock_usage(mocker):
    """
    Check create mlock usage
    """
    check_mlock_usage(
        mocker,
        'kafka_cluster_create',
        {
            'hosts': {
                'host1': get_kafka_porto_host(geo='geo1'),
                'host2': get_kafka_porto_host(geo='geo2'),
                'host3': get_kafka_porto_host(geo='geo3'),
                'host4': get_zookeeper_porto_host(geo='geo1'),
                'host5': get_zookeeper_porto_host(geo='geo2'),
                'host6': get_zookeeper_porto_host(geo='geo3'),
            },
            's3_bucket': 'test-s3-bucket',
        },
        get_state(),
    )


def test_kafka_cluster_create_alert_sync(mocker):
    """
    Check create mlock usage
    """
    check_alerts_synchronised(
        mocker,
        'kafka_cluster_create',
        {
            'hosts': {
                'host1': get_kafka_porto_host(geo='geo1'),
                'host2': get_kafka_porto_host(geo='geo2'),
                'host3': get_kafka_porto_host(geo='geo3'),
                'host4': get_zookeeper_porto_host(geo='geo1'),
                'host5': get_zookeeper_porto_host(geo='geo2'),
                'host6': get_zookeeper_porto_host(geo='geo3'),
            },
            's3_bucket': 'test-s3-bucket',
        },
        get_state(),
    )
