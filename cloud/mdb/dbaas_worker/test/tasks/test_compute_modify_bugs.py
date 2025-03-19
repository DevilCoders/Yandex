from .postgresql.utils import get_postgresql_compute_host
from ..mocks import checked_run_task_with_mocks


def test_scale_down_and_sg_changes(mocker):
    """
    Check compute scale-down + sg changes.

    More details in https://st.yandex-team.ru/MDB-13357
    Here we use the PostgreSQL cluster as an example.
    Valid changes-combinations logic shared between all clusters.
    """

    def run_task(task_type, task_args, state=None):
        return checked_run_task_with_mocks(
            mocker,
            task_type,
            task_args,
            task_params={
                'cid': 'cid-user-sg-test',
            },
            feature_flags=[],
            state=state,
        )[2]

    args = {
        'hosts': {
            'host1': get_postgresql_compute_host(geo='geo1', disk_type_id='local-ssd', space_limit=107374182400),
            'host2': get_postgresql_compute_host(geo='geo2'),
            'host3': get_postgresql_compute_host(geo='geo3'),
        },
    }
    fqdns = list(args['hosts'].keys())
    # increase CPU, cause later (in modifying task) want to decrease it
    for host in fqdns:
        args['hosts'][host]['cpu_limit'] = args['hosts'][host]['cpu_limit'] * 2

    create_state = run_task('postgresql_cluster_create', dict(**args, s3_bucket='test-s3-bucket'))

    create_state['zookeeper']['contenders'] = ['host3']
    create_state['actions'] = []
    create_state['set_actions'] = set()
    create_state['cancel_actions'] = set()

    args['zk_hosts'] = 'test-zk'

    # change security groups via task_args
    args['security_group_ids'] = ['user-sg1', 'user-sg2']
    # to cause resource changes
    for host in fqdns:
        args['hosts'][host]['cpu_limit'] = args['hosts'][host]['cpu_limit'] / 2

    run_task('postgresql_cluster_modify', args, state=create_state)
    # We don't check here, that actual modify happens,
    # cause we want to checks that given 'cluster changes' is valid in terms of the DBaaS-worker
