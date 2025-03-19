"""
Hadoop cluster create tests
"""

from test.mocks import get_state
from test.tasks.hadoop.utils import (
    get_hadoop_computenode_compute_host,
    get_hadoop_datanode_compute_host,
    get_hadoop_masternode_compute_host,
)
from test.tasks.utils import check_task_interrupt_consistency


def test_compute_hadoop_cluster_create_interrupt_consistency(mocker):
    """
    Check compute create interruptions
    """
    check_task_interrupt_consistency(
        mocker,
        'hadoop_cluster_create',
        {
            'image_id': 'hadoop-image',
            'hosts': {
                'host1': get_hadoop_masternode_compute_host(geo='geo1'),
                'host2': get_hadoop_datanode_compute_host(geo='geo2'),
                'host3': get_hadoop_computenode_compute_host(geo='geo3'),
            },
        },
        get_state(),
        feature_flags=['MDB_DATAPROC_MANAGER'],
    )
