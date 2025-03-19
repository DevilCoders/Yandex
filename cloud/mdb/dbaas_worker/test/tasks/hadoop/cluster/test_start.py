"""
Hadoop cluster start tests
"""

from test.mocks import checked_run_task_with_mocks
from test.tasks.hadoop.utils import (
    get_hadoop_computenode_compute_host,
    get_hadoop_datanode_compute_host,
    get_hadoop_masternode_compute_host,
)
from test.tasks.utils import check_task_interrupt_consistency


def test_compute_hadoop_cluster_start_interrupt_consistency(mocker):
    """
    Check compute start interruptions
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

    *_, state = checked_run_task_with_mocks(
        mocker, 'hadoop_cluster_stop', args, feature_flags=feature_flags, state=state
    )

    check_task_interrupt_consistency(
        mocker,
        'hadoop_cluster_start',
        args,
        state,
        feature_flags=feature_flags,
    )
