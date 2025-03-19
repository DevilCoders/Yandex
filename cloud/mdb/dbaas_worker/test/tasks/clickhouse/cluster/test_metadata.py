"""
ClickHouse cluster metadata tests
"""

from test.mocks import checked_run_task_with_mocks
from test.tasks.clickhouse.utils import get_clickhouse_compute_host, get_zookeeper_compute_host
from test.tasks.utils import check_mlock_usage, check_task_interrupt_consistency


def test_compute_clickhouse_metadata_update_interrupt_consistency(mocker):
    """
    Check compute metadata interruptions
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

    check_task_interrupt_consistency(
        mocker,
        'clickhouse_metadata_update',
        args,
        state,
    )


def test_clickhouse_metadata_update_mlock_usage(mocker):
    """
    Check metadata mlock usage
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

    check_mlock_usage(
        mocker,
        'clickhouse_metadata_update',
        args,
        state,
    )
