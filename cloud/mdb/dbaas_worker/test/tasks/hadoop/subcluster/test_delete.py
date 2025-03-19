"""
Hadoop subcluster delete tests
"""

from test.mocks import checked_run_task_with_mocks
from test.tasks.hadoop.utils import (
    get_hadoop_computenode_compute_host,
    get_hadoop_datanode_compute_host,
    get_hadoop_masternode_compute_host,
)
from test.tasks.utils import check_task_interrupt_consistency
from cloud.mdb.dbaas_worker.internal.tasks.hadoop.constants import DATAPROC_MANAGER_FLAG


def test_compute_hadoop_subcluster_delete_interrupt_consistency(mocker):
    """
    Check compute delete interruptions
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

    args['subcid'] = args['hosts']['host3']['subcid']
    args['host_list'] = [args['hosts']['host3'].copy()]
    args['host_list'][0]['fqdn'] = 'host3'
    del args['hosts']['host3']

    check_task_interrupt_consistency(
        mocker,
        'hadoop_subcluster_delete',
        args,
        state,
        feature_flags=feature_flags,
    )


def test_compute_hadoop_instance_group_subcluster_delete_interrupt_consistency(mocker):
    """
    Check compute instance group subcluster delete interruptions
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

    args['subcid'] = args['hosts']['host3']['subcid']
    args['host_list'] = [args['hosts']['host3'].copy()]
    args['host_list'][0]['fqdn'] = 'host3'
    del args['hosts']['host3']

    check_task_interrupt_consistency(
        mocker,
        'hadoop_subcluster_delete',
        args,
        state,
        feature_flags=feature_flags,
    )
