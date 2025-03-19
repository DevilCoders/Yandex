"""
PostgreSQL host delete tests
"""

from test.mocks import checked_run_task_with_mocks
from test.tasks.postgresql.utils import get_postgresql_compute_host, get_postgresql_porto_host
from test.tasks.utils import check_mlock_usage, check_task_interrupt_consistency, check_alerts_synchronised


def test_porto_postgresql_host_delete_interrupt_consistency(mocker):
    """
    Check porto host delete interruptions
    """
    args = {
        'hosts': {
            'host1': get_postgresql_porto_host(geo='geo1'),
            'host2': get_postgresql_porto_host(geo='geo2'),
            'host3': get_postgresql_porto_host(geo='geo3'),
        },
    }
    *_, state = checked_run_task_with_mocks(
        mocker, 'postgresql_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )

    args['zk_hosts'] = 'test-zk'
    state['zookeeper']['contenders'] = ['host3']
    state['zookeeper']['pgsync'] = {'cid-test': {'timeline': '1'}}

    args['host'] = args['hosts']['host3'].copy()
    args['host']['fqdn'] = 'host3'
    del args['hosts']['host3']

    check_task_interrupt_consistency(
        mocker,
        'postgresql_host_delete',
        args,
        state,
    )


def test_compute_postgresql_host_delete_interrupt_consistency(mocker):
    """
    Check compute host delete interruptions
    """
    args = {
        'hosts': {
            'host1': get_postgresql_compute_host(geo='geo1'),
            'host2': get_postgresql_compute_host(geo='geo2'),
            'host3': get_postgresql_compute_host(geo='geo3'),
        },
    }
    *_, state = checked_run_task_with_mocks(
        mocker, 'postgresql_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )

    args['zk_hosts'] = 'test-zk'
    state['zookeeper']['contenders'] = ['host3']
    state['zookeeper']['pgsync'] = {'cid-test': {'timeline': '1'}}

    args['host'] = args['hosts']['host3'].copy()
    args['host']['fqdn'] = 'host3'
    del args['hosts']['host3']

    check_task_interrupt_consistency(
        mocker,
        'postgresql_host_delete',
        args,
        state,
        ignore=[
            # dns changes are CAS-based, so if nothing resolves nothing is deleted
            'dns.host3.db.yandex.net-AAAA-2001:db8:3::3:removed',
        ],
    )


def test_postgresql_host_delete_mlock_usage(mocker):
    """
    Check host delete mlock usage
    """
    args = {
        'hosts': {
            'host1': get_postgresql_porto_host(geo='geo1'),
            'host2': get_postgresql_porto_host(geo='geo2'),
            'host3': get_postgresql_porto_host(geo='geo3'),
        },
    }
    *_, state = checked_run_task_with_mocks(
        mocker, 'postgresql_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )

    args['zk_hosts'] = 'test-zk'
    state['zookeeper']['contenders'] = ['host3']
    state['zookeeper']['pgsync'] = {'cid-test': {'timeline': '1'}}

    args['host'] = args['hosts']['host3'].copy()
    args['host']['fqdn'] = 'host3'
    del args['hosts']['host3']

    check_mlock_usage(
        mocker,
        'postgresql_host_delete',
        args,
        state,
    )


def test_postgresql_host_delete_alert_sync(mocker):
    """
    Check host delete mlock usage
    """
    args = {
        'hosts': {
            'host1': get_postgresql_porto_host(geo='geo1'),
            'host2': get_postgresql_porto_host(geo='geo2'),
            'host3': get_postgresql_porto_host(geo='geo3'),
        },
    }
    *_, state = checked_run_task_with_mocks(
        mocker, 'postgresql_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )

    args['zk_hosts'] = 'test-zk'
    state['zookeeper']['contenders'] = ['host3']
    state['zookeeper']['pgsync'] = {'cid-test': {'timeline': '1'}}

    args['host'] = args['hosts']['host3'].copy()
    args['host']['fqdn'] = 'host3'
    del args['hosts']['host3']

    check_alerts_synchronised(
        mocker,
        'postgresql_host_delete',
        args,
        state,
    )
