"""
MySQL cluster update tls certs tests
"""

from test.mocks import checked_run_task_with_mocks
from test.tasks.mysql.utils import get_mysql_compute_host, get_mysql_porto_host
from test.tasks.mysql.cluster.test_upgrade_80 import test_jobs, test_jobresults
from test.tasks.utils import check_mlock_usage, check_task_interrupt_consistency


def test_porto_mysql_cluster_test_maintenance_interrupt_consistency(mocker):
    """
    Check porto maintenance task interruptions
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

    args.update(
        {
            'restart': True,
            'update_tls': True,
        }
    )

    state['zookeeper']['mysql'] = {'cid-test': {'master': '"host3"'}}
    state['deploy-v2']['jobs'] = test_jobs
    state['deploy-v2']['jobresults'] = test_jobresults

    check_task_interrupt_consistency(
        mocker,
        'mysql_cluster_maintenance',
        args,
        state,
    )


def test_compute_mysql_cluster_maintenance_interrupt_consistency(mocker):
    """
    Check compute maintenance task interruptions
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

    args.update(
        {
            'restart': True,
            'update_tls': True,
        }
    )

    state['zookeeper']['mysql'] = {'cid-test': {'master': '"host3"'}}
    state['deploy-v2']['jobs'] = test_jobs
    state['deploy-v2']['jobresults'] = test_jobresults

    check_task_interrupt_consistency(
        mocker,
        'mysql_cluster_maintenance',
        args,
        state,
    )


def test_mysql_cluster_test_maintenance_mlock_usage(mocker):
    """
    Check maintenance task mlock usage
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

    args.update(
        {
            'restart': True,
            'update_tls': True,
        }
    )

    state['zookeeper']['mysql'] = {'cid-test': {'master': '"host3"'}}
    state['deploy-v2']['jobs'] = test_jobs
    state['deploy-v2']['jobresults'] = test_jobresults

    check_mlock_usage(
        mocker,
        'mysql_cluster_maintenance',
        args,
        state,
    )
