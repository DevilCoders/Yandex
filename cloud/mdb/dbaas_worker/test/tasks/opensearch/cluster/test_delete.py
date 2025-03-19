"""
OpenSearch cluster delete tests
"""

from test.mocks import checked_run_task_with_mocks
from test.tasks.opensearch.utils import (
    get_opensearch_datanode_compute_host,
    get_opensearch_datanode_porto_host,
    get_opensearch_masternode_compute_host,
    get_opensearch_masternode_porto_host,
)
from test.tasks.utils import check_mlock_usage, check_task_interrupt_consistency, check_alerts_synchronised


def test_porto_opensearch_cluster_delete_interrupt_consistency(mocker):
    """
    Check porto delete interruptions
    """
    args = {
        'hosts': {
            'host1': get_opensearch_masternode_porto_host(geo='geo1'),
            'host2': get_opensearch_datanode_porto_host(geo='geo2'),
            'host3': get_opensearch_datanode_porto_host(geo='geo3'),
        },
    }
    *_, state = checked_run_task_with_mocks(
        mocker, 'opensearch_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )

    check_task_interrupt_consistency(
        mocker,
        'opensearch_cluster_delete',
        args,
        state,
    )


def test_compute_opensearch_cluster_delete_interrupt_consistency(mocker):
    """
    Check compute delete interruptions
    """
    args = {
        'hosts': {
            'host1': get_opensearch_masternode_compute_host(geo='geo1'),
            'host2': get_opensearch_datanode_compute_host(geo='geo2'),
            'host3': get_opensearch_datanode_compute_host(geo='geo3'),
        },
    }
    *_, state = checked_run_task_with_mocks(
        mocker, 'opensearch_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )

    check_task_interrupt_consistency(
        mocker,
        'opensearch_cluster_delete',
        args,
        state,
    )


def test_opensearch_cluster_delete_mlock_usage(mocker):
    """
    Check delete mlock usage
    """
    args = {
        'hosts': {
            'host1': get_opensearch_masternode_porto_host(geo='geo1'),
            'host2': get_opensearch_datanode_porto_host(geo='geo2'),
            'host3': get_opensearch_datanode_porto_host(geo='geo3'),
        },
    }
    *_, state = checked_run_task_with_mocks(
        mocker, 'opensearch_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )

    check_mlock_usage(
        mocker,
        'opensearch_cluster_delete',
        args,
        state,
    )


def test_opensearch_cluster_delete_alert_sync(mocker):
    """
    Check delete mlock usage
    """
    args = {
        'hosts': {
            'host1': get_opensearch_masternode_porto_host(geo='geo1'),
            'host2': get_opensearch_datanode_porto_host(geo='geo2'),
            'host3': get_opensearch_datanode_porto_host(geo='geo3'),
        },
    }
    *_, state = checked_run_task_with_mocks(
        mocker, 'opensearch_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )

    check_alerts_synchronised(
        mocker,
        'opensearch_cluster_delete',
        args,
        state,
    )
