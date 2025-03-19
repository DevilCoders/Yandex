"""
Hadoop subcluster create tests
"""

from test.mocks import checked_run_task_with_mocks
from test.tasks.hadoop.utils import (
    get_hadoop_computenode_compute_host,
    get_hadoop_datanode_compute_host,
    get_hadoop_masternode_compute_host,
)
from test.tasks.utils import check_task_interrupt_consistency

from cloud.mdb.dbaas_worker.internal.tasks.hadoop.constants import DATAPROC_MANAGER_FLAG


def test_compute_hadoop_subcluster_create_interrupt_consistency(mocker):
    """
    Check compute create interruptions
    """
    args = {
        'decommission_timeout': 10,
        'image_id': 'hadoop-image',
        'hosts': {
            'host1': get_hadoop_masternode_compute_host(geo='geo1'),
            'host2': get_hadoop_datanode_compute_host(geo='geo2'),
            'host3': get_hadoop_computenode_compute_host(geo='geo3'),
        },
    }
    feature_flags = [DATAPROC_MANAGER_FLAG]
    *_, state = checked_run_task_with_mocks(mocker, 'hadoop_cluster_create', args, feature_flags=feature_flags)

    args['hosts']['host4'] = get_hadoop_datanode_compute_host(geo='geo2')
    args['hosts']['host4']['subcid'] = 'subcid-test-d2'
    args['subcid'] = 'subcid-test-d2'

    check_task_interrupt_consistency(
        mocker,
        'hadoop_subcluster_create',
        args,
        state,
        feature_flags=feature_flags,
    )


def test_compute_hadoop_instance_group_subcluster_create_interrupt_consistency(mocker):
    """
    Check compute instance group subcluster create interruptions
    """
    args = {
        'decommission_timeout': 10,
        'image_id': 'hadoop-image',
        'hosts': {
            'host1': get_hadoop_masternode_compute_host(geo='geo1'),
            'host2': get_hadoop_datanode_compute_host(geo='geo2'),
            'host3': get_hadoop_computenode_compute_host(geo='geo3'),
        },
    }
    feature_flags = [DATAPROC_MANAGER_FLAG]
    *_, state = checked_run_task_with_mocks(mocker, 'hadoop_cluster_create', args, feature_flags=feature_flags)

    args['subcid'] = 'subcid-test-c2'

    check_task_interrupt_consistency(
        mocker,
        'hadoop_subcluster_create',
        args,
        state,
        feature_flags=feature_flags,
    )
