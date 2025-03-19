"""
Greenplum cluster restore tests
"""

from test.mocks import get_state
from test.tasks.greenplum.utils import (
    get_greenplum_master_compute_host,
    get_greenplum_segment_compute_host,
    get_greenplum_master_porto_host,
    get_greenplum_segment_porto_host,
)
from test.tasks.utils import check_mlock_usage, check_task_interrupt_consistency, check_alerts_synchronised


def test_porto_greenplum_cluster_restore_interrupt_consistency(mocker):
    """
    Check porto restore interruptions
    """
    check_task_interrupt_consistency(
        mocker,
        'greenplum_cluster_restore',
        {
            'hosts': {
                'host1': get_greenplum_master_porto_host(geo='geo1'),
                'host2': get_greenplum_master_porto_host(geo='geo1'),
                'host3': get_greenplum_segment_porto_host(geo='geo1'),
                'host4': get_greenplum_segment_porto_host(geo='geo1'),
            },
            's3_bucket': 'test-s3-bucket',
        },
        get_state(),
    )


def test_compute_greenplum_cluster_restore_interrupt_consistency(mocker):
    """
    Check compute restore interruptions
    """
    check_task_interrupt_consistency(
        mocker,
        'greenplum_cluster_restore',
        {
            'hosts': {
                'host1': get_greenplum_master_compute_host(geo='geo1'),
                'host2': get_greenplum_master_compute_host(geo='geo1'),
                'host3': get_greenplum_segment_compute_host(geo='geo1'),
                'host4': get_greenplum_segment_compute_host(geo='geo1'),
            },
            's3_buckets': {'backup': 'test-s3-bucket', 'cloud_storage': 'cloud-storage-bucket'},
        },
        get_state(),
    )


def test_greenplum_cluster_restore_mlock_usage(mocker):
    """
    Check restore mlock usage
    """
    check_mlock_usage(
        mocker,
        'greenplum_cluster_restore',
        {
            'hosts': {
                'host1': get_greenplum_master_porto_host(geo='geo1'),
                'host2': get_greenplum_master_porto_host(geo='geo1'),
                'host3': get_greenplum_segment_porto_host(geo='geo1'),
                'host4': get_greenplum_segment_porto_host(geo='geo1'),
            },
            's3_bucket': 'test-s3-bucket',
        },
        get_state(),
    )
