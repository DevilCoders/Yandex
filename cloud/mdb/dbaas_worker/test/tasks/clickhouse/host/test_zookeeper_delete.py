"""
ClickHouse zookeeper host delete tests
"""

from test.mocks import checked_run_task_with_mocks
from test.tasks.clickhouse.utils import (
    get_clickhouse_compute_host,
    get_clickhouse_porto_host,
    get_zookeeper_compute_host,
    get_zookeeper_porto_host,
)
from test.tasks.utils import check_mlock_usage, check_task_interrupt_consistency, check_alerts_synchronised


def test_porto_clickhouse_zookeeper_host_delete_interrupt_consistency(mocker):
    """
    Check porto zookeeper host delete interruptions
    """
    args = {
        'hosts': {
            'host1': get_clickhouse_porto_host(geo='geo1'),
            'host2': get_clickhouse_porto_host(geo='geo2'),
            'host3': get_zookeeper_porto_host(geo='geo1'),
            'host4': get_zookeeper_porto_host(geo='geo2'),
            'host5': get_zookeeper_porto_host(geo='geo3'),
        },
    }

    *_, state = checked_run_task_with_mocks(
        mocker, 'clickhouse_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )

    args['host'] = args['hosts']['host3'].copy()
    args['host']['fqdn'] = 'host3'
    del args['hosts']['host3']
    args['zid_deleted'] = 1

    args['zk_hosts'] = ''

    check_task_interrupt_consistency(
        mocker,
        'clickhouse_zookeeper_host_delete',
        args,
        state,
    )


def test_compute_clickhouse_zookeeper_host_delete_interrupt_consistency(mocker):
    """
    Check compute zookeeper host delete interruptions
    """
    args = {
        'hosts': {
            'host1': get_clickhouse_compute_host(geo='geo1'),
            'host2': get_clickhouse_compute_host(geo='geo2'),
            'host3': get_zookeeper_compute_host(geo='geo1'),
            'host4': get_zookeeper_compute_host(geo='geo2'),
            'host5': get_zookeeper_compute_host(geo='geo3'),
        },
    }

    *_, state = checked_run_task_with_mocks(
        mocker, 'clickhouse_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )

    args['host'] = args['hosts']['host3'].copy()
    args['host']['fqdn'] = 'host3'
    del args['hosts']['host3']
    args['zid_deleted'] = 1

    args['zk_hosts'] = ''

    check_task_interrupt_consistency(
        mocker,
        'clickhouse_zookeeper_host_delete',
        args,
        state,
        ignore=[
            # dns changes are CAS-based, so if nothing resolves nothing is deleted
            'dns.host3.db.yandex.net-AAAA-2001:db8:1::1:removed',
        ],
    )


def test_clickhouse_zookeeper_host_delete_mlock_usage(mocker):
    """
    Check zookeeper host delete mlock usage
    """
    args = {
        'hosts': {
            'host1': get_clickhouse_porto_host(geo='geo1'),
            'host2': get_clickhouse_porto_host(geo='geo2'),
            'host3': get_zookeeper_porto_host(geo='geo1'),
            'host4': get_zookeeper_porto_host(geo='geo2'),
            'host5': get_zookeeper_porto_host(geo='geo3'),
        },
    }

    *_, state = checked_run_task_with_mocks(
        mocker, 'clickhouse_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )

    args['host'] = args['hosts']['host3'].copy()
    args['host']['fqdn'] = 'host3'
    del args['hosts']['host3']
    args['zid_deleted'] = 1

    args['zk_hosts'] = ''

    check_mlock_usage(
        mocker,
        'clickhouse_zookeeper_host_delete',
        args,
        state,
    )


def test_clickhouse_zookeeper_host_delete_alert_sync(mocker):
    """
    Check zookeeper host delete mlock usage
    """
    args = {
        'hosts': {
            'host1': get_clickhouse_porto_host(geo='geo1'),
            'host2': get_clickhouse_porto_host(geo='geo2'),
            'host3': get_zookeeper_porto_host(geo='geo1'),
            'host4': get_zookeeper_porto_host(geo='geo2'),
            'host5': get_zookeeper_porto_host(geo='geo3'),
        },
    }

    *_, state = checked_run_task_with_mocks(
        mocker, 'clickhouse_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )

    args['host'] = args['hosts']['host3'].copy()
    args['host']['fqdn'] = 'host3'
    del args['hosts']['host3']
    args['zid_deleted'] = 1

    args['zk_hosts'] = ''

    check_alerts_synchronised(
        mocker,
        'clickhouse_zookeeper_host_delete',
        args,
        state,
    )
