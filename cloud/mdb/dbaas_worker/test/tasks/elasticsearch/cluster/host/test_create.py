"""
ElasticSearch host create tests
"""

from test.mocks import checked_run_task_with_mocks
from test.tasks.elasticsearch.utils import (
    get_elasticsearch_datanode_compute_host,
    get_elasticsearch_datanode_porto_host,
    get_elasticsearch_masternode_compute_host,
    get_elasticsearch_masternode_porto_host,
)
from test.tasks.utils import (
    check_mlock_usage,
    check_task_interrupt_consistency,
    check_rejected,
)


def test_porto_elasticsearch_host_create_interrupt_consistency(mocker):
    """
    Check porto host create interruptions
    """
    args = {
        'hosts': {
            'host1': get_elasticsearch_datanode_porto_host(geo='geo1'),
            'host2': get_elasticsearch_datanode_porto_host(geo='geo2'),
            'host3': get_elasticsearch_masternode_porto_host(geo='geo1'),
            'host4': get_elasticsearch_masternode_porto_host(geo='geo2'),
            'host5': get_elasticsearch_masternode_porto_host(geo='geo3'),
        },
    }

    *_, state = checked_run_task_with_mocks(
        mocker, 'elasticsearch_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )

    args['hosts']['host6'] = get_elasticsearch_datanode_porto_host(geo='geo1')
    args['host'] = 'host6'

    check_task_interrupt_consistency(
        mocker,
        'elasticsearch_host_create',
        args,
        state,
    )


def test_compute_elasticsearch_host_create_interrupt_consistency(mocker):
    """
    Check compute host create interruptions
    """
    args = {
        'hosts': {
            'host1': get_elasticsearch_datanode_compute_host(geo='geo1'),
            'host2': get_elasticsearch_datanode_compute_host(geo='geo2'),
            'host3': get_elasticsearch_masternode_compute_host(geo='geo1'),
            'host4': get_elasticsearch_masternode_compute_host(geo='geo2'),
            'host5': get_elasticsearch_masternode_compute_host(geo='geo3'),
        },
    }

    *_, state = checked_run_task_with_mocks(
        mocker, 'elasticsearch_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )

    args['hosts']['host6'] = get_elasticsearch_datanode_compute_host(geo='geo1')
    args['host'] = 'host6'

    check_task_interrupt_consistency(
        mocker,
        'elasticsearch_host_create',
        args,
        state,
    )


def test_elasticsearch_host_create_mlock_usage(mocker):
    """
    Check host create mlock usage
    """
    args = {
        'hosts': {
            'host1': get_elasticsearch_datanode_porto_host(geo='geo1'),
            'host2': get_elasticsearch_datanode_porto_host(geo='geo2'),
            'host3': get_elasticsearch_masternode_porto_host(geo='geo1'),
            'host4': get_elasticsearch_masternode_porto_host(geo='geo2'),
            'host5': get_elasticsearch_masternode_porto_host(geo='geo3'),
        },
    }

    *_, state = checked_run_task_with_mocks(
        mocker, 'elasticsearch_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )

    args['hosts']['host6'] = get_elasticsearch_datanode_porto_host(geo='geo1')
    args['host'] = 'host6'

    check_mlock_usage(
        mocker,
        'elasticsearch_host_create',
        args,
        state,
    )


def test_porto_elasticsearch_host_create_revertable(mocker):
    """
    Check porto host create revertability
    """
    args = {
        'hosts': {
            'host1': get_elasticsearch_datanode_porto_host(geo='geo1'),
            'host2': get_elasticsearch_datanode_porto_host(geo='geo2'),
            'host3': get_elasticsearch_masternode_porto_host(geo='geo1'),
            'host4': get_elasticsearch_masternode_porto_host(geo='geo2'),
            'host5': get_elasticsearch_masternode_porto_host(geo='geo3'),
        },
    }

    *_, state = checked_run_task_with_mocks(
        mocker, 'elasticsearch_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )

    args['hosts']['host6'] = get_elasticsearch_datanode_porto_host(geo='geo1')
    args['host'] = 'host6'
    state['fail_actions'].add('deploy-v2-shipment-create-host1-state.sls')

    check_rejected(
        mocker,
        'elasticsearch_host_create',
        args,
        state,
    )


def test_compute_elasticsearch_host_create_revertable(mocker):
    """
    Check compute host create revertability
    """
    args = {
        'hosts': {
            'host1': get_elasticsearch_datanode_compute_host(geo='geo1'),
            'host2': get_elasticsearch_datanode_compute_host(geo='geo2'),
            'host3': get_elasticsearch_masternode_compute_host(geo='geo1'),
            'host4': get_elasticsearch_masternode_compute_host(geo='geo2'),
            'host5': get_elasticsearch_masternode_compute_host(geo='geo3'),
        },
    }

    *_, state = checked_run_task_with_mocks(
        mocker, 'elasticsearch_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )

    args['hosts']['host6'] = get_elasticsearch_datanode_compute_host(geo='geo1')
    args['host'] = 'host6'
    state['fail_actions'].add('deploy-v2-shipment-create-host1-state.sls')

    check_rejected(
        mocker,
        'elasticsearch_host_create',
        args,
        state,
    )
