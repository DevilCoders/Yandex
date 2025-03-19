"""
Kafka connector pause tests
"""

from test.mocks import checked_run_task_with_mocks
from test.tasks.kafka.utils import (
    get_kafka_compute_host,
    get_kafka_porto_host,
    get_zookeeper_compute_host,
    get_zookeeper_porto_host,
)
from test.tasks.utils import check_mlock_usage, check_task_interrupt_consistency


def test_porto_kafka_connector_pause_interrupt_consistency(mocker):
    """
    Check porto connector pause interruptions
    """
    args = {
        'hosts': {
            'host1': get_kafka_porto_host(geo='geo1'),
            'host2': get_kafka_porto_host(geo='geo2'),
            'host3': get_kafka_porto_host(geo='geo3'),
            'host4': get_zookeeper_porto_host(geo='geo1'),
            'host5': get_zookeeper_porto_host(geo='geo2'),
            'host6': get_zookeeper_porto_host(geo='geo3'),
        },
    }
    *_, state = checked_run_task_with_mocks(mocker, 'kafka_cluster_create', dict(**args, s3_bucket='test-s3-bucket'))

    args['target-connector'] = 'test-connector'

    check_task_interrupt_consistency(
        mocker,
        'kafka_connector_pause',
        args,
        state,
    )


def test_compute_kafka_connector_pause_interrupt_consistency(mocker):
    """
    Check compute connector pause interruptions
    """
    args = {
        'hosts': {
            'host1': get_kafka_compute_host(geo='geo1'),
            'host2': get_kafka_compute_host(geo='geo2'),
            'host3': get_kafka_compute_host(geo='geo3'),
            'host4': get_zookeeper_compute_host(geo='geo1'),
            'host5': get_zookeeper_compute_host(geo='geo2'),
            'host6': get_zookeeper_compute_host(geo='geo3'),
        },
    }
    *_, state = checked_run_task_with_mocks(mocker, 'kafka_cluster_create', dict(**args, s3_bucket='test-s3-bucket'))

    args['target-connector'] = 'test-connector'

    check_task_interrupt_consistency(
        mocker,
        'kafka_connector_pause',
        args,
        state,
    )


def test_kafka_connector_pause_mlock_usage(mocker):
    """
    Check connector pause mlock usage
    """
    args = {
        'hosts': {
            'host1': get_kafka_porto_host(geo='geo1'),
            'host2': get_kafka_porto_host(geo='geo2'),
            'host3': get_kafka_porto_host(geo='geo3'),
            'host4': get_zookeeper_porto_host(geo='geo1'),
            'host5': get_zookeeper_porto_host(geo='geo2'),
            'host6': get_zookeeper_porto_host(geo='geo3'),
        },
    }
    *_, state = checked_run_task_with_mocks(mocker, 'kafka_cluster_create', dict(**args, s3_bucket='test-s3-bucket'))

    args['target-connector'] = 'test-connector'

    check_mlock_usage(
        mocker,
        'kafka_connector_pause',
        args,
        state,
    )
