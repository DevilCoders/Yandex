"""
PostgreSQL cluster create tests
"""

from test.mocks import get_state, checked_run_task_with_mocks
from test.tasks.postgresql.utils import get_postgresql_compute_host


def test_compute_postgresql_cluster_create_sg_and_users(mocker):
    """
    Check compute create with SG and user sg
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
            'security_group_ids': ['user-sg1', 'user-sg2'],
        },
        get_state(),
        task_params={
            'cid': 'cid-user-sg-test',
        },
        feature_flags=feature_flags,
    )
