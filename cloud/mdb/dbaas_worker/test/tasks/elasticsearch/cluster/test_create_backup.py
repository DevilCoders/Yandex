"""
Elasticsearch cluster create backup tests
"""

from test.mocks import checked_run_task_with_mocks
from test.tasks.elasticsearch.utils import (
    get_elasticsearch_datanode_compute_host,
    get_elasticsearch_datanode_porto_host,
    get_elasticsearch_masternode_compute_host,
    get_elasticsearch_masternode_porto_host,
)
from test.tasks.utils import check_mlock_usage, check_rejected, check_task_interrupt_consistency


def test_porto_elasticsearch_cluster_create_backup_interrupt_consistency(mocker):
    """
    Check porto create backup interruptions
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

    check_task_interrupt_consistency(
        mocker,
        'elasticsearch_cluster_create_backup',
        args,
        state,
    )


def test_compute_elasticsearch_cluster_create_backup_interrupt_consistency(mocker):
    """
    Check compute create backup interruptions
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

    check_task_interrupt_consistency(
        mocker,
        'elasticsearch_cluster_create_backup',
        args,
        state,
    )


def test_elasticsearch_cluster_create_backup_mlock_usage(mocker):
    """
    Check create backup mlock usage
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

    check_mlock_usage(
        mocker,
        'elasticsearch_cluster_create_backup',
        args,
        state,
    )


def test_elasticsearch_cluster_create_backup_revertable(mocker):
    """
    Check create backup revertability
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

    state['fail_actions'].add('deploy-v2-shipment-get-host1-state.sls')

    check_rejected(
        mocker,
        'elasticsearch_cluster_create_backup',
        args,
        state,
    )
