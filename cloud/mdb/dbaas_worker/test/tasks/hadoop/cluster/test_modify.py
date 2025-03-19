"""
Hadoop cluster modify tests
"""
from hamcrest import assert_that, has_entries, has_items

from test.mocks import checked_run_task_with_mocks
from test.tasks.hadoop.utils import (
    get_hadoop_computenode_compute_host,
    get_hadoop_datanode_compute_host,
    get_hadoop_masternode_compute_host,
)
from test.tasks.utils import check_task_interrupt_consistency


def test_compute_hadoop_cluster_modify_interrupt_consistency(mocker):
    """
    Check compute modify interruptions
    """
    args = {
        'image_id': 'hadoop-image',
        'decommission_timeout': 10,
        'hosts': {
            'host1': get_hadoop_masternode_compute_host(geo='geo1'),
            'host2': get_hadoop_datanode_compute_host(geo='geo2'),
            'host3': get_hadoop_computenode_compute_host(geo='geo3'),
        },
    }
    feature_flags = ['MDB_DATAPROC_MANAGER']
    *_, state = checked_run_task_with_mocks(mocker, 'hadoop_cluster_create', args, feature_flags=feature_flags)

    args['hosts']['host1']['cpu_limit'] = args['hosts']['host1']['cpu_limit'] * 2
    args['hosts']['host1']['space_limit'] = args['hosts']['host1']['space_limit'] * 2

    interrupted_state = check_task_interrupt_consistency(
        mocker,
        'hadoop_cluster_modify',
        args,
        state,
        feature_flags=feature_flags,
    )

    for hostname in ['host1', 'host2', 'host3']:
        assert_that(
            interrupted_state['compute']['instances'].values(),
            has_items(has_entries({'fqdn': hostname, 'status': 'RUNNING'})),
        )
