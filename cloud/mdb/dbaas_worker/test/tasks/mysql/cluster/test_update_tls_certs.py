"""
MySQL cluster update tls certs tests
"""

from test.mocks import checked_run_task_with_mocks
from test.tasks.mysql.utils import get_mysql_compute_host, get_mysql_porto_host
from test.tasks.utils import check_mlock_usage, check_task_interrupt_consistency


def test_porto_mysql_cluster_test_update_tls_certs_interrupt_consistency(mocker):
    """
    Check porto update tls certs interruptions
    """
    args = {
        'hosts': {
            'host1': get_mysql_porto_host(geo='geo1'),
            'host2': get_mysql_porto_host(geo='geo2'),
            'host3': get_mysql_porto_host(geo='geo3'),
        },
        'zk_hosts': 'test-zk',
    }
    *_, state = checked_run_task_with_mocks(mocker, 'mysql_cluster_create', dict(**args, s3_bucket='test-s3-bucket'))

    check_task_interrupt_consistency(
        mocker,
        'mysql_cluster_update_tls_certs',
        args,
        state,
    )


def test_compute_mysql_cluster_update_tls_certs_interrupt_consistency(mocker):
    """
    Check compute update tls certs interruptions
    """
    args = {
        'hosts': {
            'host1': get_mysql_compute_host(geo='geo1'),
            'host2': get_mysql_compute_host(geo='geo2'),
            'host3': get_mysql_compute_host(geo='geo3'),
        },
        'zk_hosts': 'test-zk',
    }
    *_, state = checked_run_task_with_mocks(mocker, 'mysql_cluster_create', dict(**args, s3_bucket='test-s3-bucket'))

    check_task_interrupt_consistency(
        mocker,
        'mysql_cluster_update_tls_certs',
        args,
        state,
    )


def test_mysql_cluster_test_update_tls_certs_mlock_usage(mocker):
    """
    Check update tls certs mlock usage
    """
    args = {
        'hosts': {
            'host1': get_mysql_porto_host(geo='geo1'),
            'host2': get_mysql_porto_host(geo='geo2'),
            'host3': get_mysql_porto_host(geo='geo3'),
        },
        'zk_hosts': 'test-zk',
    }
    *_, state = checked_run_task_with_mocks(mocker, 'mysql_cluster_create', dict(**args, s3_bucket='test-s3-bucket'))

    check_mlock_usage(
        mocker,
        'mysql_cluster_update_tls_certs',
        args,
        state,
    )
