"""
PostgreSQL cluster metadata tests
"""

from test.mocks import checked_run_task_with_mocks
from test.tasks.postgresql.utils import get_postgresql_compute_host
from test.tasks.utils import check_mlock_usage, check_task_interrupt_consistency


def test_compute_postgresql_metadata_update_interrupt_consistency(mocker):
    """
    Check compute metadata interruptions
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

    check_task_interrupt_consistency(
        mocker,
        'postgresql_metadata_update',
        args,
        state,
    )


def test_postgresql_metadata_update_mlock_usage(mocker):
    """
    Check metadata mlock usage
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

    check_mlock_usage(
        mocker,
        'postgresql_metadata_update',
        args,
        state,
    )
