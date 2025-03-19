"""
PostgreSQL cluster add host test
"""

from test.mocks import checked_run_task_with_mocks
from test.tasks.postgresql.utils import get_postgresql_compute_host


def test_compute_postgresql_cluster_add_host_with_sg(mocker):
    """
    Check compute add host with sg
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
        feature_flags=feature_flags,
    )

    state['zookeeper']['contenders'] = ['host3']
    state['actions'] = []
    state['set_actions'] = set()
    state['cancel_actions'] = set()

    for i in range(1, 4):
        args['hosts'][f'host{i}']['vtype_id'] = f'instance-{i}'
    # add new host
    args['hosts']['host4'] = get_postgresql_compute_host(geo='geo2', disk_type_id='local-ssd', space_limit=107374182400)
    args['host'] = args['hosts']['host4']

    checked_run_task_with_mocks(
        mocker,
        'postgresql_host_create',
        args,
        state=state,
        feature_flags=feature_flags,
    )
