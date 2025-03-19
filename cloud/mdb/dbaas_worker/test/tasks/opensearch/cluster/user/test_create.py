"""
Opensearch user create tests
"""

from test.mocks import checked_run_task_with_mocks
from test.tasks.opensearch.utils import (
    get_opensearch_datanode_porto_host,
    get_opensearch_masternode_porto_host,
    get_opensearch_masternode_compute_host,
    get_opensearch_datanode_compute_host,
)

from test.tasks.utils import check_mlock_usage, check_task_interrupt_consistency


def test_porto_opensearch_user_create_interrupt_consistency(mocker):
    """
    Check porto user create interruptions
    """
    args = {
        'hosts': {
            'host1': get_opensearch_datanode_porto_host(geo='geo1'),
            'host2': get_opensearch_datanode_porto_host(geo='geo2'),
            'host3': get_opensearch_datanode_porto_host(geo='geo3'),
            'host4': get_opensearch_masternode_porto_host(geo='geo1'),
            'host5': get_opensearch_masternode_porto_host(geo='geo2'),
            'host6': get_opensearch_masternode_porto_host(geo='geo3'),
        },
    }
    *_, state = checked_run_task_with_mocks(
        mocker, 'opensearch_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )

    args['target-user'] = 'test-user'

    check_task_interrupt_consistency(
        mocker,
        'opensearch_user_create',
        args,
        state,
    )


def test_compute_opensearch_user_create_interrupt_consistency(mocker):
    """
    Check compute user create interruptions
    """
    args = {
        'hosts': {
            'host1': get_opensearch_datanode_compute_host(geo='geo1'),
            'host2': get_opensearch_datanode_compute_host(geo='geo2'),
            'host3': get_opensearch_datanode_compute_host(geo='geo3'),
            'host4': get_opensearch_masternode_compute_host(geo='geo1'),
            'host5': get_opensearch_masternode_compute_host(geo='geo2'),
            'host6': get_opensearch_masternode_compute_host(geo='geo3'),
        },
    }
    *_, state = checked_run_task_with_mocks(
        mocker, 'opensearch_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )

    args['target-user'] = 'test-user'

    check_task_interrupt_consistency(
        mocker,
        'opensearch_user_create',
        args,
        state,
    )


def test_opensearch_user_create_mlock_usage(mocker):
    """
    Check user create mlock usage
    """
    args = {
        'hosts': {
            'host1': get_opensearch_datanode_porto_host(geo='geo1'),
            'host2': get_opensearch_datanode_porto_host(geo='geo2'),
            'host3': get_opensearch_datanode_porto_host(geo='geo3'),
            'host4': get_opensearch_masternode_porto_host(geo='geo1'),
            'host5': get_opensearch_masternode_porto_host(geo='geo2'),
            'host6': get_opensearch_masternode_porto_host(geo='geo3'),
        },
    }
    *_, state = checked_run_task_with_mocks(
        mocker, 'opensearch_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )

    args['target-user'] = 'test-user'

    check_mlock_usage(
        mocker,
        'opensearch_user_create',
        args,
        state,
    )
