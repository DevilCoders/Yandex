"""
PostgreSQL cluster create tests
"""

from test.mocks import get_state
from test.tasks.postgresql.utils import get_postgresql_compute_host, get_postgresql_porto_host
from test.tasks.utils import check_mlock_usage, check_task_interrupt_consistency, check_alerts_synchronised

from cloud.mdb.dbaas_worker.internal.tasks.postgresql.cluster.create import CREATE, RESTORE, make_master_pillar


class Test_make_master_pillar:
    def test_args_for_create(self):
        """
        Should add do-backup
        """
        assert make_master_pillar(CREATE, {}) == {'do-backup': True}

    def test_args_for_restore(self):
        """
        Should add target-pillar-id and restore-from and do-backup
        """
        assert make_master_pillar(RESTORE, {'target-pillar-id': '333', 'restore-from': {'cid': '111-222'}}) == {
            'do-backup': True,
            'target-pillar-id': '333',
            'restore-from': {
                'cid': '111-222',
            },
        }


def test_porto_postgresql_cluster_create_interrupt_consistency(mocker):
    """
    Check porto create interruptions
    """
    check_task_interrupt_consistency(
        mocker,
        'postgresql_cluster_create',
        {
            'hosts': {
                'host1': get_postgresql_porto_host(geo='geo1'),
                'host2': get_postgresql_porto_host(geo='geo2'),
                'host3': get_postgresql_porto_host(geo='geo3'),
            },
            's3_bucket': 'test-s3-bucket',
        },
        get_state(),
    )


def test_compute_postgresql_cluster_create_interrupt_consistency(mocker):
    """
    Check compute create interruptions
    """
    check_task_interrupt_consistency(
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
    )


def test_postgresql_cluster_create_mlock_usage(mocker):
    """
    Check create mlock usage
    """
    check_mlock_usage(
        mocker,
        'postgresql_cluster_create',
        {
            'hosts': {
                'host1': get_postgresql_porto_host(geo='geo1'),
                'host2': get_postgresql_porto_host(geo='geo2'),
                'host3': get_postgresql_porto_host(geo='geo3'),
            },
            's3_bucket': 'test-s3-bucket',
        },
        get_state(),
    )


def test_postgresql_cluster_create_alert_sync(mocker):
    """
    Check create mlock usage
    """
    check_alerts_synchronised(
        mocker,
        'postgresql_cluster_create',
        {
            'hosts': {
                'host1': get_postgresql_porto_host(geo='geo1'),
                'host2': get_postgresql_porto_host(geo='geo2'),
                'host3': get_postgresql_porto_host(geo='geo3'),
            },
            's3_bucket': 'test-s3-bucket',
        },
        get_state(),
    )
