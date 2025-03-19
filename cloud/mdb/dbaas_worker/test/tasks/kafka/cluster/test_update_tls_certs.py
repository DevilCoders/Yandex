"""
Kafka cluster update tls certs tests
"""

from test.mocks import checked_run_task_with_mocks
from test.tasks.kafka.utils import (
    get_kafka_compute_host,
    get_kafka_porto_host,
    get_zookeeper_compute_host,
    get_zookeeper_porto_host,
)
from test.tasks.utils import check_mlock_usage, check_task_interrupt_consistency


def test_porto_kafka_cluster_update_tls_certs_interrupt_consistency(mocker):
    """
    Check porto update tls certs interruptions
    """
    args = {
        'hosts': {
            'host1': get_kafka_porto_host(geo='geo1'),
            'host2': get_kafka_porto_host(geo='geo2'),
            'host3': get_zookeeper_porto_host(geo='geo1'),
            'host4': get_zookeeper_porto_host(geo='geo2'),
            'host5': get_zookeeper_porto_host(geo='geo3'),
        },
    }

    *_, state = checked_run_task_with_mocks(mocker, 'kafka_cluster_create', dict(**args, s3_bucket='test-s3-bucket'))

    check_task_interrupt_consistency(
        mocker,
        'kafka_cluster_update_tls_certs',
        args,
        state,
    )


def test_compute_kafka_cluster_update_tls_certs_interrupt_consistency(mocker):
    """
    Check compute update tls certs interruptions
    """
    args = {
        'hosts': {
            'host1': get_kafka_compute_host(geo='geo1'),
            'host2': get_kafka_compute_host(geo='geo2'),
            'host3': get_zookeeper_compute_host(geo='geo1'),
            'host4': get_zookeeper_compute_host(geo='geo2'),
            'host5': get_zookeeper_compute_host(geo='geo3'),
        },
    }

    *_, state = checked_run_task_with_mocks(mocker, 'kafka_cluster_create', dict(**args, s3_bucket='test-s3-bucket'))

    check_task_interrupt_consistency(
        mocker,
        'kafka_cluster_update_tls_certs',
        args,
        state,
    )


def test_kafka_cluster_update_tls_certs_mlock_usage(mocker):
    """
    Check update tls certs mlock usage
    """
    args = {
        'hosts': {
            'host1': get_kafka_porto_host(geo='geo1'),
            'host2': get_kafka_porto_host(geo='geo2'),
            'host3': get_zookeeper_porto_host(geo='geo1'),
            'host4': get_zookeeper_porto_host(geo='geo2'),
            'host5': get_zookeeper_porto_host(geo='geo3'),
        },
    }

    *_, state = checked_run_task_with_mocks(mocker, 'kafka_cluster_create', dict(**args, s3_bucket='test-s3-bucket'))

    check_mlock_usage(
        mocker,
        'kafka_cluster_update_tls_certs',
        args,
        state,
    )
