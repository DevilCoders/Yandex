"""
Greenplum cluster purge tests
"""

from test.mocks import checked_run_task_with_mocks
from test.tasks.greenplum.utils import (
    get_greenplum_master_compute_host,
    get_greenplum_segment_compute_host,
    get_greenplum_master_porto_host,
    get_greenplum_segment_porto_host,
)
from test.tasks.utils import check_task_interrupt_consistency


def test_porto_greenplum_cluster_purge_interrupt_consistency(mocker):
    """
    Check porto purge interruptions
    """
    args = {
        'hosts': {
            'host1': get_greenplum_master_porto_host(geo='geo1'),
            'host2': get_greenplum_master_porto_host(geo='geo1'),
            'host3': get_greenplum_segment_porto_host(geo='geo1'),
            'host4': get_greenplum_segment_porto_host(geo='geo1'),
        },
        's3_bucket': 'test-s3-bucket',
    }

    *_, state = checked_run_task_with_mocks(mocker, 'greenplum_cluster_create', args)

    *_, state = checked_run_task_with_mocks(mocker, 'greenplum_cluster_delete', args, state=state)

    *_, state = checked_run_task_with_mocks(mocker, 'greenplum_cluster_delete_metadata', args, state=state)

    state['s3']['test-s3-bucket']['uploads'].append({'Key': 'partial', 'UploadId': 'test-upload-id'})
    state['s3']['test-s3-bucket']['files'].append('finished')

    check_task_interrupt_consistency(
        mocker,
        'greenplum_cluster_purge',
        args,
        state,
    )


def test_compute_greenplum_cluster_purge_interrupt_consistency(mocker):
    """
    Check compute purge interruptions
    """
    args = {
        'hosts': {
            'host1': get_greenplum_master_compute_host(geo='geo1'),
            'host2': get_greenplum_master_compute_host(geo='geo1'),
            'host3': get_greenplum_segment_compute_host(geo='geo1'),
            'host4': get_greenplum_segment_compute_host(geo='geo1'),
        },
        's3_bucket': 'test-s3-bucket',
    }

    *_, state = checked_run_task_with_mocks(mocker, 'greenplum_cluster_create', args)

    *_, state = checked_run_task_with_mocks(mocker, 'greenplum_cluster_delete', args, state=state)

    *_, state = checked_run_task_with_mocks(mocker, 'greenplum_cluster_delete_metadata', args, state=state)

    state['s3']['test-s3-bucket']['uploads'].append({'Key': 'partial', 'UploadId': 'test-upload-id'})
    state['s3']['test-s3-bucket']['files'].append('finished')

    check_task_interrupt_consistency(
        mocker,
        'greenplum_cluster_purge',
        args,
        state,
    )
