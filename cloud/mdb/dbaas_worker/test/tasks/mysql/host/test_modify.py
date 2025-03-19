"""
MySQL host modify tests
"""

from test.mocks import checked_run_task_with_mocks
from test.tasks.mysql.utils import get_mysql_compute_host, get_mysql_porto_host
from test.tasks.utils import check_mlock_usage, check_task_interrupt_consistency


def test_porto_mysql_host_modify_interrupt_consistency(mocker):
    """
    Check porto host modify interruptions
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

    state['zookeeper']['mysql'] = {'cid-test': {'master': '"host3"', 'maintenance': {'mysync_paused': True}}}

    args['ha_status_changed'] = True
    args['restart'] = True

    args['host'] = {'fqdn': 'host3'}

    check_task_interrupt_consistency(
        mocker,
        'mysql_host_modify',
        args,
        state,
    )


def test_compute_mysql_host_modify_interrupt_consistency(mocker):
    """
    Check compute host modify interruptions
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

    state['zookeeper']['mysql'] = {'cid-test': {'master': '"host3"', 'maintenance': {'mysync_paused': True}}}

    args['ha_status_changed'] = True
    args['restart'] = True

    args['host'] = {'fqdn': 'host3'}

    check_task_interrupt_consistency(
        mocker,
        'mysql_host_modify',
        args,
        state,
    )


def test_mysql_host_modify_mlock_usage(mocker):
    """
    Check host modify mlock usage
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

    state['zookeeper']['mysql'] = {'cid-test': {'master': '"host3"', 'maintenance': {'mysync_paused': True}}}

    args['ha_status_changed'] = True
    args['restart'] = True

    args['host'] = {'fqdn': 'host3'}

    check_mlock_usage(
        mocker,
        'mysql_host_modify',
        args,
        state,
    )
