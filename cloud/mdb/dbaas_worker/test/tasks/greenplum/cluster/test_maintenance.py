"""
Greenplum cluster maintenance executor tests
"""

from test.mocks import checked_run_task_with_mocks
from test.tasks.greenplum.utils import (
    get_greenplum_master_compute_host,
    get_greenplum_segment_compute_host,
    get_greenplum_master_porto_host,
    get_greenplum_segment_porto_host,
)
from test.tasks.utils import check_mlock_usage, check_task_interrupt_consistency


def test_porto_greenplum_cluster_test_maintenance_interrupt_consistency(mocker):
    """
    Check porto maintenance interruptions
    """
    args = {
        'hosts': {
            'host1': get_greenplum_master_porto_host(geo='geo1'),
            'host2': get_greenplum_master_porto_host(geo='geo1'),
            'host3': get_greenplum_segment_porto_host(geo='geo1'),
            'host4': get_greenplum_segment_porto_host(geo='geo1'),
        },
        'restart': True,
        'update_tls': True,
        'force_tls_certs': True,
    }
    *_, state = checked_run_task_with_mocks(
        mocker, 'greenplum_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )

    check_task_interrupt_consistency(
        mocker,
        'greenplum_cluster_maintenance',
        args,
        state,
    )


def test_compute_greenplum_cluster_maintenance_interrupt_consistency(mocker):
    """
    Check compute maintenance interruptions
    """
    args = {
        'hosts': {
            'host1': get_greenplum_master_compute_host(geo='geo1'),
            'host2': get_greenplum_master_compute_host(geo='geo1'),
            'host3': get_greenplum_segment_compute_host(geo='geo1'),
            'host4': get_greenplum_segment_compute_host(geo='geo1'),
        },
        'restart': True,
        'update_tls': True,
        'force_tls_certs': True,
    }
    *_, state = checked_run_task_with_mocks(
        mocker, 'greenplum_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )

    check_task_interrupt_consistency(
        mocker,
        'greenplum_cluster_maintenance',
        args,
        state,
    )


def test_greenplum_cluster_test_maintenance_mlock_usage(mocker):
    """
    Check maintenance mlock usage
    """
    args = {
        'hosts': {
            'host1': get_greenplum_master_porto_host(geo='geo1'),
            'host2': get_greenplum_master_porto_host(geo='geo1'),
            'host3': get_greenplum_segment_porto_host(geo='geo1'),
            'host4': get_greenplum_segment_porto_host(geo='geo1'),
        },
        'restart': True,
        'update_tls': True,
        'force_tls_certs': True,
    }
    *_, state = checked_run_task_with_mocks(
        mocker, 'greenplum_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )

    check_mlock_usage(
        mocker,
        'greenplum_cluster_maintenance',
        args,
        state,
    )
