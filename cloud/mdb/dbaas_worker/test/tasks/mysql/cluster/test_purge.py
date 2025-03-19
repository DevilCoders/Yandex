"""
MySQL cluster purge tests
"""

from test.mocks import checked_run_task_with_mocks
from test.tasks.mysql.utils import get_mysql_compute_host, get_mysql_porto_host
from test.tasks.utils import check_task_interrupt_consistency


def test_porto_mysql_cluster_purge_interrupt_consistency(mocker):
    """
    Check porto purge interruptions
    """
    args = {
        'hosts': {
            'host1': get_mysql_porto_host(geo='geo1'),
            'host2': get_mysql_porto_host(geo='geo2'),
            'host3': get_mysql_porto_host(geo='geo3'),
        },
        's3_bucket': 'test-s3-bucket',
        'zk_hosts': 'test-zk',
    }
    *_, state = checked_run_task_with_mocks(mocker, 'mysql_cluster_create', args)

    *_, state = checked_run_task_with_mocks(mocker, 'mysql_cluster_delete', args, state=state)

    *_, state = checked_run_task_with_mocks(mocker, 'mysql_cluster_delete_metadata', args, state=state)

    state['s3']['test-s3-bucket']['uploads'].append({'Key': 'partial', 'UploadId': 'test-upload-id'})
    state['s3']['test-s3-bucket']['files'].append('finished')

    check_task_interrupt_consistency(
        mocker,
        'mysql_cluster_purge',
        args,
        state,
    )


def test_compute_mysql_cluster_purge_interrupt_consistency(mocker):
    """
    Check compute purge interruptions
    """
    args = {
        'hosts': {
            'host1': get_mysql_compute_host(geo='geo1'),
            'host2': get_mysql_compute_host(geo='geo2'),
            'host3': get_mysql_compute_host(geo='geo3'),
        },
        's3_bucket': 'test-s3-bucket',
        'zk_hosts': 'test-zk',
    }
    *_, state = checked_run_task_with_mocks(mocker, 'mysql_cluster_create', args)

    *_, state = checked_run_task_with_mocks(mocker, 'mysql_cluster_delete', args, state=state)

    *_, state = checked_run_task_with_mocks(mocker, 'mysql_cluster_delete_metadata', args, state=state)

    state['s3']['test-s3-bucket']['uploads'].append({'Key': 'partial', 'UploadId': 'test-upload-id'})
    state['s3']['test-s3-bucket']['files'].append('finished')

    check_task_interrupt_consistency(
        mocker,
        'mysql_cluster_purge',
        args,
        state,
    )
