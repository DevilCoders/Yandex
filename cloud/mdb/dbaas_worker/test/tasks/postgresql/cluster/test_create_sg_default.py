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
    state = get_state()
    state['metadb']['queries'].append(
        {
            'query': 'get_alerts_by_cid',
            'kwargs': {'cid': 'cid-user-sg-test'},
            'result': [
                {
                    'alert_group_id': '1',
                    'folder_ext_id': 'compute-folder-id',
                    'warning_threshold': '1',
                    'critical_threshold': '1',
                    'status': 'DELETING',
                    'notification_channels': [],
                    'alert_ext_id': 'test-deletion-id',
                    'template_id': '2',
                    'monitoring_folder_id': 'solomon-project',
                    'template_version': 'test-ver-1',
                },
                {
                    'alert_group_id': '1',
                    'folder_ext_id': 'compute-folder-id',
                    'warning_threshold': '1',
                    'critical_threshold': '1',
                    'status': 'CREATING',
                    'notification_channels': [],
                    'alert_ext_id': '',
                    'template_id': '1',
                    'monitoring_folder_id': 'solomon-project',
                    'template_version': 'test-ver-1',
                },
            ],
        }
    )
    _, _, state = checked_run_task_with_mocks(
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
        state,
        task_params={
            'cid': 'cid-user-sg-test',
        },
        feature_flags=feature_flags,
    )

    for i in range(1, 4):
        for iface in state['compute']['instances'][f'instance-{i}']['networkInterfaces']:
            if iface['index'] == 0:
                expect_sg = ['sg_id_service_cid_cid-user-sg-test', 'default-sg-id']
                assert set(iface['securityGroupIds']) == set(expect_sg)
            else:
                assert set(iface['securityGroupIds']) == set()
