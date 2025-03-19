"""
Elasticsearch user delete tests
"""

from test.mocks import checked_run_task_with_mocks
from test.tasks.elasticsearch.utils import (
    get_elasticsearch_datanode_porto_host,
    get_elasticsearch_masternode_porto_host,
    get_elasticsearch_masternode_compute_host,
    get_elasticsearch_datanode_compute_host,
)
from test.tasks.utils import check_mlock_usage, check_task_interrupt_consistency


def test_porto_elasticsearch_user_delete_interrupt_consistency(mocker):
    """
    Check porto user delete interruptions
    """
    args = {
        'hosts': {
            'host1': get_elasticsearch_datanode_porto_host(geo='geo1'),
            'host2': get_elasticsearch_datanode_porto_host(geo='geo2'),
            'host3': get_elasticsearch_datanode_porto_host(geo='geo3'),
            'host4': get_elasticsearch_masternode_porto_host(geo='geo1'),
            'host5': get_elasticsearch_masternode_porto_host(geo='geo2'),
            'host6': get_elasticsearch_masternode_porto_host(geo='geo3'),
        },
    }
    *_, state = checked_run_task_with_mocks(
        mocker, 'elasticsearch_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )

    args['target-user'] = 'test-user'

    check_task_interrupt_consistency(
        mocker,
        'elasticsearch_user_modify',
        args,
        state,
    )


def test_compute_elasticsearch_user_delete_interrupt_consistency(mocker):
    """
    Check compute user delete interruptions
    """
    args = {
        'hosts': {
            'host1': get_elasticsearch_datanode_compute_host(geo='geo1'),
            'host2': get_elasticsearch_datanode_compute_host(geo='geo2'),
            'host3': get_elasticsearch_datanode_compute_host(geo='geo3'),
            'host4': get_elasticsearch_masternode_compute_host(geo='geo1'),
            'host5': get_elasticsearch_masternode_compute_host(geo='geo2'),
            'host6': get_elasticsearch_masternode_compute_host(geo='geo3'),
        },
    }
    *_, state = checked_run_task_with_mocks(
        mocker, 'elasticsearch_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )

    args['target-user'] = 'test-user'

    check_task_interrupt_consistency(
        mocker,
        'elasticsearch_user_modify',
        args,
        state,
    )


def test_elasticsearch_user_delete_mlock_usage(mocker):
    """
    Check user delete mlock usage
    """
    args = {
        'hosts': {
            'host1': get_elasticsearch_datanode_porto_host(geo='geo1'),
            'host2': get_elasticsearch_datanode_porto_host(geo='geo2'),
            'host3': get_elasticsearch_datanode_porto_host(geo='geo3'),
            'host4': get_elasticsearch_masternode_porto_host(geo='geo1'),
            'host5': get_elasticsearch_masternode_porto_host(geo='geo2'),
            'host6': get_elasticsearch_masternode_porto_host(geo='geo3'),
        },
    }
    *_, state = checked_run_task_with_mocks(
        mocker, 'elasticsearch_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )

    args['target-user'] = 'test-user'

    check_mlock_usage(
        mocker,
        'elasticsearch_user_modify',
        args,
        state,
    )
