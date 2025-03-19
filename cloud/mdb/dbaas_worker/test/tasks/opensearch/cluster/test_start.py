"""
Opensearch cluster start tests
"""

from test.mocks import checked_run_task_with_mocks
from test.tasks.opensearch.utils import (
    get_opensearch_datanode_compute_host,
    get_opensearch_masternode_compute_host,
)
from test.tasks.utils import check_mlock_usage, check_task_interrupt_consistency


def test_compute__cluster_start_interrupt_consistency(mocker):
    """
    Check compute start interruptions
    """
    args = {
        'hosts': {
            'host1': get_opensearch_datanode_compute_host(geo='geo1'),
            'host2': get_opensearch_datanode_compute_host(geo='geo2'),
            'host3': get_opensearch_masternode_compute_host(geo='geo1'),
            'host4': get_opensearch_masternode_compute_host(geo='geo2'),
            'host5': get_opensearch_masternode_compute_host(geo='geo3'),
        },
    }

    *_, state = checked_run_task_with_mocks(
        mocker, 'opensearch_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )

    *_, state = checked_run_task_with_mocks(mocker, 'opensearch_cluster_stop', args, state=state)

    check_task_interrupt_consistency(
        mocker,
        'opensearch_cluster_start',
        args,
        state,
    )


def test_opensearch_cluster_start_mlock_usage(mocker):
    """
    Check start mlock usage
    """
    args = {
        'hosts': {
            'host1': get_opensearch_datanode_compute_host(geo='geo1'),
            'host2': get_opensearch_datanode_compute_host(geo='geo2'),
            'host3': get_opensearch_masternode_compute_host(geo='geo1'),
            'host4': get_opensearch_masternode_compute_host(geo='geo2'),
            'host5': get_opensearch_masternode_compute_host(geo='geo3'),
        },
    }

    *_, state = checked_run_task_with_mocks(
        mocker, 'opensearch_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )

    *_, state = checked_run_task_with_mocks(mocker, 'opensearch_cluster_stop', args, state=state)

    check_mlock_usage(
        mocker,
        'opensearch_cluster_start',
        args,
        state,
    )
