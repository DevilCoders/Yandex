"""
PostgreSQL set user sg for created cluster
"""

from test.mocks import checked_run_task_with_mocks
from test.tasks.postgresql.utils import get_postgresql_compute_host


def test_compute_postgresql_cluster_set_user_sg_for_created_cluster(mocker):
    """
    Check added user sg set for all hosts
    """
    args = {
        'hosts': {
            'host1': get_postgresql_compute_host(geo='geo1', disk_type_id='local-ssd', space_limit=107374182400),
            'host2': get_postgresql_compute_host(geo='geo2'),
            'host3': get_postgresql_compute_host(geo='geo3'),
        },
    }
    feature_flags = []
    _, _, state = checked_run_task_with_mocks(
        mocker,
        'postgresql_cluster_create',
        dict(**args, s3_bucket='test-s3-bucket'),
        task_params={
            'cid': 'cid-user-sg-test',
        },
        feature_flags=feature_flags,
    )

    state['zookeeper']['contenders'] = ['host3']
    state['actions'] = []
    state['set_actions'] = set()
    state['cancel_actions'] = set()

    # set instance id of exist hosts
    for i in range(1, 4):
        args['hosts'][f'host{i}']['vtype_id'] = f'instance-{i}'

    args['zk_hosts'] = 'test-zk'

    args['security_group_ids'] = ['user-sg1', 'user-sg2']

    _, _, state = checked_run_task_with_mocks(
        mocker,
        'postgresql_cluster_modify',
        args,
        state=state,
        task_params={
            'cid': 'cid-user-sg-test',
        },
        feature_flags=feature_flags,
    )

    for i in range(1, 4):
        for iface in state['compute']['instances'][f'instance-{i}']['networkInterfaces']:
            if iface['index'] == 0:
                expect_sg = ['sg_id_service_cid_cid-user-sg-test', 'user-sg1', 'user-sg2']
                assert set(iface['securityGroupIds']) == set(expect_sg)
            else:
                assert set(iface['securityGroupIds']) == set()
