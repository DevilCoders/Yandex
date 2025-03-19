"""
Greenplum cluster stop tests
"""

from test.mocks import checked_run_task_with_mocks
from test.tasks.greenplum.utils import (
    get_greenplum_master_compute_host,
    get_greenplum_segment_compute_host,
)
from test.tasks.utils import check_mlock_usage, check_task_interrupt_consistency


def test_compute_greenplum_cluster_stop_interrupt_consistency(mocker):
    """
    Check compute stop interruptions
    """
    args = {
        'hosts': {
            'host1': get_greenplum_master_compute_host(geo='geo1'),
            'host2': get_greenplum_segment_compute_host(geo='geo1'),
            'host3': get_greenplum_segment_compute_host(geo='geo1'),
        },
    }

    *_, state = checked_run_task_with_mocks(
        mocker, 'greenplum_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )

    check_task_interrupt_consistency(
        mocker,
        'greenplum_cluster_stop',
        args,
        state,
        # dns changes are CAS-based, so if nothing resolves nothing is deleted
        ignore=[
            'dns.host1.db.yandex.net-AAAA-2001:db8:1::1:removed',
            'dns.host2.db.yandex.net-AAAA-2001:db8:1::2:removed',
            'dns.host3.db.yandex.net-AAAA-2001:db8:1::3:removed',
            'compute.instance.instance-2.STOPPED:wait ok',
            'compute.instance.instance-3.STOPPED:wait ok',
            'compute.instance.instance-1.STOPPED:wait ok',
            'instance.instance-1:stop initiated',
            'instance.instance-3:stop initiated',
            'instance.instance-2:stop initiated',
        ],
    )


def test_greenplum_cluster_stop_mlock_usage(mocker):
    """
    Check stop mlock usage
    """
    args = {
        'hosts': {
            'host1': get_greenplum_master_compute_host(geo='geo1'),
            'host2': get_greenplum_segment_compute_host(geo='geo1'),
            'host3': get_greenplum_segment_compute_host(geo='geo1'),
        },
    }
    *_, state = checked_run_task_with_mocks(
        mocker, 'greenplum_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )

    check_mlock_usage(
        mocker,
        'greenplum_cluster_stop',
        args,
        state,
    )
