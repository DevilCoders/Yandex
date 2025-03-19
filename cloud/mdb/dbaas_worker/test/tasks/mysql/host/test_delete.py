"""
MySQL host delete tests
"""

from test.mocks import checked_run_task_with_mocks
from test.tasks.mysql.utils import get_mysql_compute_host, get_mysql_porto_host
from test.tasks.utils import check_mlock_usage, check_task_interrupt_consistency, check_alerts_synchronised


def test_porto_mysql_host_delete_interrupt_consistency(mocker):
    """
    Check porto host delete interruptions
    """
    args = {
        'hosts': {
            'host1': get_mysql_porto_host(geo='geo1'),
            'host2': get_mysql_porto_host(geo='geo2'),
            'host3': get_mysql_porto_host(geo='geo3'),
        },
        'zk_hosts': 'test-zk',
    }
    *_, state = checked_run_task_with_mocks(mocker, 'mysql_cluster_create', dict(**args, s3_bucket='test-s3-bucket'))

    state['zookeeper']['mysql'] = {
        'cid-test': {
            'master': '"host3"',
            'active_nodes': '["host1","host2","host3"]',
            'last_switch': {
                'result': {
                    'ok': True,
                },
                'initiated_by': 'test-fqdn',
                'initiated_at': 'test-timestamp',
            },
        },
    }

    args['host'] = args['hosts']['host3'].copy()
    args['host']['fqdn'] = 'host3'
    del args['hosts']['host3']

    check_task_interrupt_consistency(
        mocker,
        'mysql_host_delete',
        args,
        state,
    )


def test_compute_mysql_host_delete_interrupt_consistency(mocker):
    """
    Check compute host delete interruptions
    """
    args = {
        'hosts': {
            'host1': get_mysql_compute_host(geo='geo1'),
            'host2': get_mysql_compute_host(geo='geo2'),
            'host3': get_mysql_compute_host(geo='geo3'),
        },
        'zk_hosts': 'test-zk',
    }
    *_, state = checked_run_task_with_mocks(mocker, 'mysql_cluster_create', dict(**args, s3_bucket='test-s3-bucket'))

    state['zookeeper']['mysql'] = {
        'cid-test': {
            'master': '"host3"',
            'active_nodes': '["host1","host2","host3"]',
            'last_switch': {
                'result': {
                    'ok': True,
                },
                'initiated_by': 'test-fqdn',
                'initiated_at': 'test-timestamp',
            },
        },
    }

    args['host'] = args['hosts']['host3'].copy()
    args['host']['fqdn'] = 'host3'
    del args['hosts']['host3']

    check_task_interrupt_consistency(
        mocker,
        'mysql_host_delete',
        args,
        state,
        ignore=[
            # dns changes are CAS-based, so if nothing resolves nothing is deleted
            'dns.host3.db.yandex.net-AAAA-2001:db8:3::3:removed',
        ],
    )


def test_mysql_host_delete_mlock_usage(mocker):
    """
    Check host delete mlock usage
    """
    args = {
        'hosts': {
            'host1': get_mysql_porto_host(geo='geo1'),
            'host2': get_mysql_porto_host(geo='geo2'),
            'host3': get_mysql_porto_host(geo='geo3'),
        },
        'zk_hosts': 'test-zk',
    }
    *_, state = checked_run_task_with_mocks(mocker, 'mysql_cluster_create', dict(**args, s3_bucket='test-s3-bucket'))

    state['zookeeper']['mysql'] = {
        'cid-test': {
            'master': '"host3"',
            'active_nodes': '["host1","host2","host3"]',
            'last_switch': {
                'result': {
                    'ok': True,
                },
                'initiated_by': 'test-fqdn',
                'initiated_at': 'test-timestamp',
            },
        },
    }

    args['host'] = args['hosts']['host3'].copy()
    args['host']['fqdn'] = 'host3'
    del args['hosts']['host3']

    check_mlock_usage(
        mocker,
        'mysql_host_delete',
        args,
        state,
    )


def test_mysql_host_delete_alert_sync(mocker):
    """
    Check host delete mlock usage
    """
    args = {
        'hosts': {
            'host1': get_mysql_porto_host(geo='geo1'),
            'host2': get_mysql_porto_host(geo='geo2'),
            'host3': get_mysql_porto_host(geo='geo3'),
        },
        'zk_hosts': 'test-zk',
    }
    *_, state = checked_run_task_with_mocks(mocker, 'mysql_cluster_create', dict(**args, s3_bucket='test-s3-bucket'))

    state['zookeeper']['mysql'] = {
        'cid-test': {
            'master': '"host3"',
            'active_nodes': '["host1","host2","host3"]',
            'last_switch': {
                'result': {
                    'ok': True,
                },
                'initiated_by': 'test-fqdn',
                'initiated_at': 'test-timestamp',
            },
        },
    }

    args['host'] = args['hosts']['host3'].copy()
    args['host']['fqdn'] = 'host3'
    del args['hosts']['host3']

    check_alerts_synchronised(
        mocker,
        'mysql_host_delete',
        args,
        state,
    )
