"""
PostgreSQL cluster start failover tests
"""

from test.mocks import checked_run_task_with_mocks
from test.tasks.greenplum.utils import (
    get_greenplum_master_compute_host,
    get_greenplum_segment_compute_host,
    get_greenplum_master_porto_host,
    get_greenplum_segment_porto_host,
)
from test.tasks.utils import check_mlock_usage, check_task_interrupt_consistency


def test_porto_greenplum_cluster_start_failover_interrupt_consistency(mocker):
    """
    Check porto start failover interruptions
    """
    args = {
        'hosts': {
            'host1': get_greenplum_master_porto_host(geo='geo1'),
            'host2': get_greenplum_segment_porto_host(geo='geo2'),
            'host3': get_greenplum_segment_porto_host(geo='geo3'),
        },
        'target_host': 'host2',
    }
    *_, state = checked_run_task_with_mocks(
        mocker, 'greenplum_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )

    check_task_interrupt_consistency(
        mocker,
        'greenplum_cluster_start_segment_failover',
        args,
        state,
    )


def test_compute_greenplum_cluster_segment_failover_interrupt_consistency(mocker):
    """
    Check compute start failover interruptions
    """
    args = {
        'hosts': {
            'host1': get_greenplum_master_compute_host(geo='geo1'),
            'host2': get_greenplum_segment_compute_host(geo='geo2'),
            'host3': get_greenplum_segment_compute_host(geo='geo3'),
        },
        'target_host': 'host2',
    }
    *_, state = checked_run_task_with_mocks(
        mocker, 'greenplum_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )

    check_task_interrupt_consistency(
        mocker,
        'greenplum_cluster_start_segment_failover',
        args,
        state,
    )


def test_greenplum_cluster_segment_failover_mlock_usage(mocker):
    """
    Check start failover mlock usage
    """
    args = {
        'hosts': {
            'host1': get_greenplum_master_porto_host(geo='geo1'),
            'host2': get_greenplum_segment_porto_host(geo='geo2'),
            'host3': get_greenplum_segment_porto_host(geo='geo3'),
        },
        'target_host': 'host2',
    }
    *_, state = checked_run_task_with_mocks(
        mocker, 'greenplum_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )

    check_mlock_usage(
        mocker,
        'greenplum_cluster_start_segment_failover',
        args,
        state,
    )
