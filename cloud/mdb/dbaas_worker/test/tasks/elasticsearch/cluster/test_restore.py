"""
ElasticSearch cluster restore tests
"""

from test.mocks import get_state
from test.tasks.elasticsearch.utils import (
    get_elasticsearch_datanode_compute_host,
    get_elasticsearch_datanode_porto_host,
    get_elasticsearch_masternode_compute_host,
    get_elasticsearch_masternode_porto_host,
)
from test.tasks.utils import check_mlock_usage, check_task_interrupt_consistency, check_alerts_synchronised


def test_porto_elasticsearch_cluster_restore_interrupt_consistency(mocker):
    """
    Check porto restore interruptions
    """
    check_task_interrupt_consistency(
        mocker,
        'elasticsearch_cluster_restore',
        {
            'hosts': {
                'host1': get_elasticsearch_masternode_porto_host(geo='geo1'),
                'host2': get_elasticsearch_datanode_porto_host(geo='geo2'),
                'host3': get_elasticsearch_datanode_porto_host(geo='geo3'),
            },
            's3_buckets': {'backup': 'test-s3-bucket'},
            'restore_from': {'bucket': 'restore-s3-bucket', 'backup': 'backup-id'},
        },
        get_state(),
    )


def test_compute_elasticsearch_cluster_restore_interrupt_consistency(mocker):
    """
    Check compute restore interruptions
    """
    check_task_interrupt_consistency(
        mocker,
        'elasticsearch_cluster_restore',
        {
            'hosts': {
                'host1': get_elasticsearch_masternode_compute_host(geo='geo1'),
                'host2': get_elasticsearch_datanode_compute_host(geo='geo2'),
                'host3': get_elasticsearch_datanode_compute_host(geo='geo3'),
            },
            's3_buckets': {'secure_backups': 'test-s3-bucket'},
            'restore_from': {'bucket': 'restore-s3-bucket', 'backup': 'backup-id'},
        },
        get_state(),
    )


def test_elasticsearch_cluster_restore_mlock_usage(mocker):
    """
    Check restore mlock usage
    """
    check_mlock_usage(
        mocker,
        'elasticsearch_cluster_restore',
        {
            'hosts': {
                'host1': get_elasticsearch_masternode_porto_host(geo='geo1'),
                'host2': get_elasticsearch_datanode_porto_host(geo='geo2'),
                'host3': get_elasticsearch_datanode_porto_host(geo='geo3'),
            },
            's3_buckets': {'secure_backups': 'test-s3-bucket'},
            'restore_from': {'bucket': 'restore-s3-bucket', 'backup': 'backup-id'},
        },
        get_state(),
    )


def test_elasticsearch_cluster_restore_alert_sync(mocker):
    """
    Check create mlock usage
    """
    check_alerts_synchronised(
        mocker,
        'elasticsearch_cluster_restore',
        {
            'hosts': {
                'host1': get_elasticsearch_masternode_porto_host(geo='geo1'),
                'host2': get_elasticsearch_datanode_porto_host(geo='geo2'),
                'host3': get_elasticsearch_datanode_porto_host(geo='geo3'),
            },
            's3_buckets': {'secure_backups': 'test-s3-bucket'},
            'restore_from': {'bucket': 'restore-s3-bucket', 'backup': 'backup-id'},
        },
        get_state(),
    )
