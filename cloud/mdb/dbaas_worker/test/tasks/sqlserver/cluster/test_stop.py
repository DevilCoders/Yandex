"""
SQLServer cluster stop tests
"""

from test.mocks import checked_run_task_with_mocks
from test.tasks.sqlserver.utils import get_sqlserver_compute_host
from test.tasks.utils import check_mlock_usage, check_task_interrupt_consistency


def test_compute_sqlserver_cluster_stop_interrupt_consistency(mocker):
    """
    Check compute stop interruptions
    """
    args = {
        'hosts': {
            'host1': get_sqlserver_compute_host(geo='geo1'),
            'host2': get_sqlserver_compute_host(geo='geo2'),
            'host3': get_sqlserver_compute_host(geo='geo3'),
        },
    }
    *_, state = checked_run_task_with_mocks(
        mocker, 'sqlserver_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )

    check_task_interrupt_consistency(
        mocker,
        'sqlserver_cluster_stop',
        args,
        state,
        # dns changes are CAS-based, so if nothing resolves nothing is deleted
        ignore=[
            'dns.host1.db.yandex.net-AAAA-2001:db8:1::1:removed',
            'dns.host2.db.yandex.net-AAAA-2001:db8:2::2:removed',
            'dns.host3.db.yandex.net-AAAA-2001:db8:3::3:removed',
            'instance.instance-1:stop initiated',
            'compute.instance.instance-2.STOPPED:wait ok',
            'compute.instance.instance-3.STOPPED:wait ok',
            'compute.instance.instance-1.STOPPED:wait ok',
            'instance.instance-2:stop initiated',
            'instance.instance-3:stop initiated',
        ],
    )


def test_sqlserver_cluster_stop_mlock_usage(mocker):
    """
    Check stop mlock usage
    """
    args = {
        'hosts': {
            'host1': get_sqlserver_compute_host(geo='geo1'),
            'host2': get_sqlserver_compute_host(geo='geo2'),
            'host3': get_sqlserver_compute_host(geo='geo3'),
        },
    }
    *_, state = checked_run_task_with_mocks(
        mocker, 'sqlserver_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )

    check_mlock_usage(
        mocker,
        'sqlserver_cluster_stop',
        args,
        state,
    )
