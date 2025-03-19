"""
Kafka cluster upgrade tests
"""

from test.mocks import checked_run_task_with_mocks
from test.tasks.kafka.utils import (
    get_kafka_porto_host,
    get_kafka_compute_host,
    get_zookeeper_porto_host,
    get_zookeeper_compute_host,
)
from test.tasks.utils import check_mlock_usage, check_task_interrupt_consistency


def test_porto_kafka_cluster_upgrade_interrupt_consistency(mocker):
    """
    Check porto upgrade interruptions
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

    *_, state = checked_run_task_with_mocks(mocker, 'kafka_cluster_create', args)

    state['metadb']['queries'] = [
        {
            'query': 'get_pillar',
            'kwargs': {'cid': 'cid-test', 'path': ['data', 'kafka']},
            'result': [{'value': {'version': '2.8', 'inter_broker_protocol_version': '2.6'}}],
        },
    ] + state['metadb']['queries']

    check_task_interrupt_consistency(
        mocker,
        'kafka_cluster_upgrade',
        args,
        state,
    )


def test_compute_kafka_cluster_upgrade_interrupt_consistency(mocker):
    """
    Check compute upgrade interruptions
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

    *_, state = checked_run_task_with_mocks(mocker, 'kafka_cluster_create', args)

    state['metadb']['queries'] = [
        {
            'query': 'get_pillar',
            'kwargs': {'cid': 'cid-test', 'path': ['data', 'kafka']},
            'result': [{'value': {'version': '2.8', 'inter_broker_protocol_version': '2.6'}}],
        },
    ] + state['metadb']['queries']

    check_task_interrupt_consistency(
        mocker,
        'kafka_cluster_upgrade',
        args,
        state,
    )


def test_kafka_cluster_upgrade_mlock_usage(mocker):
    """
    Check upgrade mlock usage
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

    *_, state = checked_run_task_with_mocks(mocker, 'kafka_cluster_create', args)

    state['metadb']['queries'] = [
        {
            'query': 'get_pillar',
            'kwargs': {'cid': 'cid-test', 'path': ['data', 'kafka']},
            'result': [{'value': {'version': '2.8', 'inter_broker_protocol_version': '2.6'}}],
        },
    ] + state['metadb']['queries']

    check_mlock_usage(
        mocker,
        'kafka_cluster_upgrade',
        args,
        state,
    )
