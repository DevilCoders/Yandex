"""
Greenplum cluster start tests
"""

from test.mocks import checked_run_task_with_mocks
from test.tasks.greenplum.utils import (
    get_greenplum_master_compute_host,
    get_greenplum_segment_compute_host,
)
from test.tasks.utils import check_mlock_usage, check_task_interrupt_consistency


def test_compute__cluster_start_interrupt_consistency(mocker):
    """
    Check compute start interruptions
    """
    args = {
        'hosts': {
            'host1': get_greenplum_master_compute_host(geo='geo1'),
            'host2': get_greenplum_master_compute_host(geo='geo1'),
            'host3': get_greenplum_segment_compute_host(geo='geo1'),
            'host4': get_greenplum_segment_compute_host(geo='geo1'),
            'host5': get_greenplum_segment_compute_host(geo='geo1'),
        },
    }

    *_, state = checked_run_task_with_mocks(
        mocker, 'greenplum_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )

    *_, state = checked_run_task_with_mocks(mocker, 'greenplum_cluster_stop', args, state=state)

    check_task_interrupt_consistency(
        mocker,
        'greenplum_cluster_start',
        args,
        state,
    )


def test_greenplum_cluster_start_mlock_usage(mocker):
    """
    Check start mlock usage
    """
    args = {
        'hosts': {
            'host1': get_greenplum_master_compute_host(geo='geo1'),
            'host2': get_greenplum_master_compute_host(geo='geo1'),
            'host3': get_greenplum_segment_compute_host(geo='geo1'),
            'host4': get_greenplum_segment_compute_host(geo='geo1'),
            'host5': get_greenplum_segment_compute_host(geo='geo1'),
        },
    }

    *_, state = checked_run_task_with_mocks(
        mocker, 'greenplum_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )

    *_, state = checked_run_task_with_mocks(mocker, 'greenplum_cluster_stop', args, state=state)

    check_mlock_usage(
        mocker,
        'greenplum_cluster_start',
        args,
        state,
    )
