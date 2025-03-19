"""
Greenplum cluster update tls certs tests
"""

from test.mocks import checked_run_task_with_mocks
from test.tasks.greenplum.utils import (
    get_greenplum_master_compute_host,
    get_greenplum_segment_compute_host,
    get_greenplum_master_porto_host,
    get_greenplum_segment_porto_host,
)
from test.tasks.utils import check_mlock_usage, check_task_interrupt_consistency


def test_porto_greenplum_cluster_test_update_tls_certs_interrupt_consistency(mocker):
    """
    Check porto update tls certs interruptions
    """
    args = {
        'hosts': {
            'host1': get_greenplum_master_porto_host(geo='geo1'),
            'host2': get_greenplum_master_porto_host(geo='geo1'),
            'host3': get_greenplum_segment_porto_host(geo='geo1'),
            'host4': get_greenplum_segment_porto_host(geo='geo1'),
        },
    }
    *_, state = checked_run_task_with_mocks(
        mocker, 'greenplum_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )

    check_task_interrupt_consistency(
        mocker,
        'greenplum_cluster_update_tls_certs',
        args,
        state,
    )


def test_compute_greenplum_cluster_update_tls_certs_interrupt_consistency(mocker):
    """
    Check compute update tls certs interruptions
    """
    args = {
        'hosts': {
            'host1': get_greenplum_master_compute_host(geo='geo1'),
            'host2': get_greenplum_master_compute_host(geo='geo1'),
            'host3': get_greenplum_segment_compute_host(geo='geo1'),
            'host4': get_greenplum_segment_compute_host(geo='geo1'),
        },
    }
    *_, state = checked_run_task_with_mocks(
        mocker, 'greenplum_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )

    check_task_interrupt_consistency(
        mocker,
        'greenplum_cluster_update_tls_certs',
        args,
        state,
    )


def test_greenplum_cluster_test_update_tls_certs_mlock_usage(mocker):
    """
    Check update tls certs mlock usage
    """
    args = {
        'hosts': {
            'host1': get_greenplum_master_porto_host(geo='geo1'),
            'host2': get_greenplum_master_porto_host(geo='geo1'),
            'host3': get_greenplum_segment_porto_host(geo='geo1'),
            'host4': get_greenplum_segment_porto_host(geo='geo1'),
        },
    }
    *_, state = checked_run_task_with_mocks(
        mocker, 'greenplum_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )

    check_mlock_usage(
        mocker,
        'greenplum_cluster_update_tls_certs',
        args,
        state,
    )
