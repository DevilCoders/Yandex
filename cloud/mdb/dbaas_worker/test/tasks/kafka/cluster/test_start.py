"""
Apache Kafka cluster start tests
"""

from test.mocks import checked_run_task_with_mocks
from test.tasks.kafka.utils import get_kafka_compute_host, get_zookeeper_compute_host
from test.tasks.utils import check_mlock_usage, check_task_interrupt_consistency


def test_compute_kafka_cluster_start_interrupt_consistency(mocker):
    """
    Check compute start interruptions
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

    *_, state = checked_run_task_with_mocks(mocker, 'kafka_cluster_stop', args, state=state)

    check_task_interrupt_consistency(
        mocker,
        'kafka_cluster_start',
        args,
        state,
    )


def test_kafka_cluster_start_mlock_usage(mocker):
    """
    Check start mlock usage
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

    *_, state = checked_run_task_with_mocks(mocker, 'kafka_cluster_stop', args, state=state)

    check_mlock_usage(
        mocker,
        'kafka_cluster_start',
        args,
        state,
    )
