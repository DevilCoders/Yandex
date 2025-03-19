"""
PostgreSQL cluster create tests
"""

from test.mocks import get_state, checked_run_task_with_mocks
from test.tasks.postgresql.utils import get_postgresql_compute_host


def test_compute_postgresql_cluster_create_sg(mocker):
    """
    Check compute create with sg
    """
    feature_flags = []
    checked_run_task_with_mocks(
        mocker,
        'postgresql_cluster_create',
        {
            'hosts': {
                'host1': get_postgresql_compute_host(geo='geo1'),
                'host2': get_postgresql_compute_host(geo='geo2'),
                'host3': get_postgresql_compute_host(geo='geo3'),
            },
            's3_bucket': 'test-s3-bucket',
        },
        get_state(),
        feature_flags=feature_flags,
    )
