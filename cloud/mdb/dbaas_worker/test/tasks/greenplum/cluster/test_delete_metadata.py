"""
Greenplum cluster delete metadata tests
"""

from test.mocks import checked_run_task_with_mocks
from test.tasks.greenplum.utils import (
    get_greenplum_master_compute_host,
    get_greenplum_segment_compute_host,
    get_greenplum_master_porto_host,
    get_greenplum_segment_porto_host,
)
from test.tasks.utils import check_task_interrupt_consistency


def test_porto_greenplum_cluster_delete_metadata_interrupt_consistency(mocker):
    """
    Check porto delete interruptions
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

    *_, state = checked_run_task_with_mocks(mocker, 'greenplum_cluster_delete', args, state=state)

    check_task_interrupt_consistency(
        mocker,
        'greenplum_cluster_delete_metadata',
        args,
        state,
    )


def test_compute_greenplum_cluster_delete_metadata_interrupt_consistency(mocker):
    """
    Check compute delete interruptions
    """
    args = {
        'hosts': {
            'host1': get_greenplum_master_compute_host(geo='geo1'),
            'host2': get_greenplum_master_compute_host(geo='geo1'),
            'host3': get_greenplum_segment_compute_host(geo='geo1'),
            'host4': get_greenplum_segment_compute_host(geo='geo1'),
        },
        's3_buckets': {
            'backup': 'backup-bucket',
            'cloud_storage': 'cloud-storage-bucket',
        },
    }

    *_, state = checked_run_task_with_mocks(mocker, 'greenplum_cluster_create', args)

    *_, state = checked_run_task_with_mocks(mocker, 'greenplum_cluster_delete', args, state=state)

    check_task_interrupt_consistency(
        mocker,
        'greenplum_cluster_delete_metadata',
        args,
        state,
        ignore=[
            # dns changes are CAS-based, so if nothing resolves nothing is deleted
            'dns.host1.db.yandex.net-AAAA-2001:db8:1::1:removed',
            'dns.host2.db.yandex.net-AAAA-2001:db8:1::2:removed',
            'dns.host3.db.yandex.net-AAAA-2001:db8:1::3:removed',
            'dns.host4.db.yandex.net-AAAA-2001:db8:1::4:removed',
        ],
    )
