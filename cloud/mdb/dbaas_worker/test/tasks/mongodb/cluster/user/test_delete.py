"""
mongodb user delete tests
"""

from test.mocks import checked_run_task_with_mocks
from test.tasks.mongodb.utils import get_mongod_compute_host, get_mongod_porto_host
from test.tasks.utils import check_mlock_usage, check_task_interrupt_consistency


def test_porto_mongodb_user_delete_interrupt_consistency(mocker):
    """
    Check porto user delete interruptions
    """
    args = {
        'hosts': {
            'host1': get_mongod_porto_host(geo='geo1'),
            'host2': get_mongod_porto_host(geo='geo2'),
            'host3': get_mongod_porto_host(geo='geo3'),
        },
    }
    *_, state = checked_run_task_with_mocks(mocker, 'mongodb_cluster_create', dict(**args, s3_bucket='test-s3-bucket'))

    args['target-user'] = 'test-user'

    check_task_interrupt_consistency(
        mocker,
        'mongodb_user_delete',
        args,
        state,
    )


def test_compute_mongodb_user_delete_interrupt_consistency(mocker):
    """
    Check compute user delete interruptions
    """
    args = {
        'hosts': {
            'host1': get_mongod_compute_host(geo='geo1'),
            'host2': get_mongod_compute_host(geo='geo2'),
            'host3': get_mongod_compute_host(geo='geo3'),
        },
    }
    *_, state = checked_run_task_with_mocks(mocker, 'mongodb_cluster_create', dict(**args, s3_bucket='test-s3-bucket'))

    args['target-user'] = 'test-user'

    check_task_interrupt_consistency(
        mocker,
        'mongodb_user_delete',
        args,
        state,
    )


def test_mongodb_user_delete_mlock_usage(mocker):
    """
    Check user delete mlock usage
    """
    args = {
        'hosts': {
            'host1': get_mongod_porto_host(geo='geo1'),
            'host2': get_mongod_porto_host(geo='geo2'),
            'host3': get_mongod_porto_host(geo='geo3'),
        },
    }
    *_, state = checked_run_task_with_mocks(mocker, 'mongodb_cluster_create', dict(**args, s3_bucket='test-s3-bucket'))

    args['target-user'] = 'test-user'

    check_mlock_usage(
        mocker,
        'mongodb_user_delete',
        args,
        state,
    )
