"""
ElasticSearch host delete tests
"""

from test.mocks import checked_run_task_with_mocks
from test.tasks.elasticsearch.utils import (
    get_elasticsearch_datanode_compute_host,
    get_elasticsearch_datanode_porto_host,
    get_elasticsearch_masternode_compute_host,
    get_elasticsearch_masternode_porto_host,
)
from test.tasks.utils import check_mlock_usage, check_task_interrupt_consistency, check_alerts_synchronised


def test_porto_elasticsearch_host_delete_interrupt_consistency(mocker):
    """
    Check porto host delete interruptions
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

    args['host'] = args['hosts']['host1'].copy()
    args['host']['fqdn'] = 'host1'
    del args['hosts']['host1']

    check_task_interrupt_consistency(
        mocker,
        'elasticsearch_host_delete',
        args,
        state,
    )


def test_compute_elasticsearch_host_delete_interrupt_consistency(mocker):
    """
    Check compute host delete interruptions
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

    args['host'] = args['hosts']['host1'].copy()
    args['host']['fqdn'] = 'host1'
    del args['hosts']['host1']

    check_task_interrupt_consistency(
        mocker,
        'elasticsearch_host_delete',
        args,
        state,
        ignore=[
            # dns changes are CAS-based, so if nothing resolves nothing is deleted
            'dns.host1.db.yandex.net-AAAA-2001:db8:1::4:removed',
        ],
    )


def test_elasticsearch_host_delete_mlock_usage(mocker):
    """
    Check host delete mlock usage
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

    args['host'] = args['hosts']['host1'].copy()
    args['host']['fqdn'] = 'host1'
    del args['hosts']['host1']

    check_mlock_usage(
        mocker,
        'elasticsearch_host_delete',
        args,
        state,
    )


def test_elasticsearch_host_delete_alert_sync(mocker):
    """
    Check host delete mlock usage
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

    args['host'] = args['hosts']['host1'].copy()
    args['host']['fqdn'] = 'host1'
    del args['hosts']['host1']

    check_alerts_synchronised(
        mocker,
        'elasticsearch_host_delete',
        args,
        state,
    )
