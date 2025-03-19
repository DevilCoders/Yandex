"""
Hadoop task test utils
"""

from test.tasks.utils import get_compute_task_host
from cloud.mdb.dbaas_worker.internal.tasks.hadoop.utils import MASTER_ROLE_TYPE, DATA_ROLE_TYPE, COMPUTE_ROLE_TYPE


def get_hadoop_masternode_compute_host(**fields):
    """
    Get hadoop masternode compute host for task args
    """
    ret = get_compute_task_host(**fields)

    ret.update(
        dict(
            subcid='subcid-test-m',
            roles=[MASTER_ROLE_TYPE],
            cluster_type='hadoop_cluster',
        )
    )

    return ret


def get_hadoop_datanode_compute_host(**fields):
    """
    Get hadoop datanode compute host for task args
    """
    ret = get_compute_task_host(**fields)

    ret.update(
        dict(
            subcid='subcid-test-d',
            roles=[DATA_ROLE_TYPE],
            cluster_type='hadoop_cluster',
        )
    )

    return ret


def get_hadoop_computenode_compute_host(**fields):
    """
    Get hadoop computenode compute host for task args
    """
    ret = get_compute_task_host(**fields)

    ret.update(
        dict(
            subcid='subcid-test-c',
            roles=[COMPUTE_ROLE_TYPE],
            cluster_type='hadoop_cluster',
        )
    )

    return ret
