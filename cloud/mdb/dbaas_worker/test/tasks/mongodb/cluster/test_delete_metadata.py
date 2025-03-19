"""
MongoDB cluster delete metadata tests
"""

from test.mocks import checked_run_task_with_mocks
from test.tasks.mongodb.utils import get_mongod_compute_host, get_mongod_porto_host
from test.tasks.utils import check_task_interrupt_consistency


def test_porto_mongodb_cluster_delete_metadata_interrupt_consistency(mocker):
    """
    Check porto delete metadata interruptions
    """
    args = {
        'hosts': {
            'host1': get_mongod_porto_host(geo='geo1'),
            'host2': get_mongod_porto_host(geo='geo2'),
            'host3': get_mongod_porto_host(geo='geo3'),
        },
    }
    *_, state = checked_run_task_with_mocks(mocker, 'mongodb_cluster_create', dict(**args, s3_bucket='test-s3-bucket'))

    *_, state = checked_run_task_with_mocks(mocker, 'mongodb_cluster_delete', args, state=state)

    args['zk_hosts'] = 'test-zk'

    check_task_interrupt_consistency(
        mocker,
        'mongodb_cluster_delete_metadata',
        args,
        state,
    )


def test_compute_mongodb_cluster_delete_metadata_interrupt_consistency(mocker):
    """
    Check compute delete metadata interruptions
    """
    args = {
        'hosts': {
            'host1': get_mongod_compute_host(geo='geo1'),
            'host2': get_mongod_compute_host(geo='geo2'),
            'host3': get_mongod_compute_host(geo='geo3'),
        },
    }
    *_, state = checked_run_task_with_mocks(mocker, 'mongodb_cluster_create', dict(**args, s3_bucket='test-s3-bucket'))

    *_, state = checked_run_task_with_mocks(mocker, 'mongodb_cluster_delete', args, state=state)

    args['zk_hosts'] = 'test-zk'

    check_task_interrupt_consistency(
        mocker,
        'mongodb_cluster_delete_metadata',
        args,
        state,
        ignore=[
            # dns changes are CAS-based, so if nothing resolves nothing is deleted
            'dns.host1.db.yandex.net-AAAA-2001:db8:1::1:removed',
            'dns.host2.db.yandex.net-AAAA-2001:db8:2::2:removed',
            'dns.host3.db.yandex.net-AAAA-2001:db8:3::3:removed',
        ],
    )
