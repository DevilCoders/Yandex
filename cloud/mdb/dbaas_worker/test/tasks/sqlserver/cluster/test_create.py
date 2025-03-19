"""
SQLServer cluster create tests
"""

from test.mocks import get_state
from test.tasks.sqlserver.utils import get_sqlserver_compute_host, get_witness_compute_host
from test.tasks.utils import check_mlock_usage, check_task_interrupt_consistency, check_alerts_synchronised


def test_compute_sqlserver_cluster_create_interrupt_consistency(mocker):
    """
    Check compute create interruptions
    """
    check_task_interrupt_consistency(
        mocker,
        'sqlserver_cluster_create',
        {
            'hosts': {
                'host1': get_sqlserver_compute_host(geo='geo1'),
                'host2': get_sqlserver_compute_host(geo='geo2'),
                'host3': get_sqlserver_compute_host(geo='geo3'),
            },
            's3_bucket': 'test-s3-bucket',
        },
        get_state(),
    )


def test_compute_sqlserver_2node_cluster_create_interrupt_consistency(mocker):
    """
    Check compute create interruptions
    """
    check_task_interrupt_consistency(
        mocker,
        'sqlserver_cluster_create',
        {
            'hosts': {
                'host1': get_sqlserver_compute_host(geo='geo1'),
                'host2': get_sqlserver_compute_host(geo='geo2'),
                'host3': get_witness_compute_host(geo='geo3'),
            },
            's3_bucket': 'test-s3-bucket',
        },
        get_state(),
    )


def test_sqlserver_cluster_create_alert_sync(mocker):
    """
    Check create mlock usage
    """
    check_alerts_synchronised(
        mocker,
        'sqlserver_cluster_create',
        {
            'hosts': {
                'host1': get_sqlserver_compute_host(geo='geo1'),
                'host2': get_sqlserver_compute_host(geo='geo2'),
                'host3': get_sqlserver_compute_host(geo='geo3'),
            },
            's3_bucket': 'test-s3-bucket',
        },
        get_state(),
    )


def test_sqlserver_2node_cluster_create_alert_sync(mocker):
    """
    Check create mlock usage
    """
    check_alerts_synchronised(
        mocker,
        'sqlserver_cluster_create',
        {
            'hosts': {
                'host1': get_sqlserver_compute_host(geo='geo1'),
                'host2': get_sqlserver_compute_host(geo='geo2'),
                'host3': get_witness_compute_host(geo='geo3'),
            },
            's3_bucket': 'test-s3-bucket',
        },
        get_state(),
    )


def test_sqlserver_cluster_create_mlock_usage(mocker):
    """
    Check create mlock usage
    """
    check_mlock_usage(
        mocker,
        'sqlserver_cluster_create',
        {
            'hosts': {
                'host1': get_sqlserver_compute_host(geo='geo1'),
                'host2': get_sqlserver_compute_host(geo='geo2'),
                'host3': get_sqlserver_compute_host(geo='geo3'),
            },
            's3_bucket': 'test-s3-bucket',
        },
        get_state(),
    )


def test_sqlserver_2node_cluster_create_mlock_usage(mocker):
    """
    Check create mlock usage
    """
    check_mlock_usage(
        mocker,
        'sqlserver_cluster_create',
        {
            'hosts': {
                'host1': get_sqlserver_compute_host(geo='geo1'),
                'host2': get_sqlserver_compute_host(geo='geo2'),
                'host3': get_witness_compute_host(geo='geo3'),
            },
            's3_bucket': 'test-s3-bucket',
        },
        get_state(),
    )
