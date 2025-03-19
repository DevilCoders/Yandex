"""
Opensearch task test utils
"""

from test.tasks.utils import get_compute_task_host, get_porto_task_host


def get_opensearch_datanode_porto_host(**fields):
    """
    Get opensearch datanode porto host for task args
    """
    ret = get_porto_task_host(**fields)

    ret.update(dict(roles=['opensearch_cluster.datanode']))

    return ret


def get_opensearch_masternode_porto_host(**fields):
    """
    Get opensearch masternode porto host for task args
    """
    ret = get_porto_task_host(**fields)

    ret.update(dict(roles=['opensearch_cluster.masternode']))

    return ret


def get_opensearch_datanode_compute_host(**fields):
    """
    Get opensearch datanode compute host for task args
    """
    ret = get_compute_task_host(**fields)

    ret.update(
        dict(
            roles=['opensearch_cluster.datanode'],
            cluster_type='opensearch_cluster',
        )
    )

    return ret


def get_opensearch_masternode_compute_host(**fields):
    """
    Get opensearch masternode compute host for task args
    """
    ret = get_compute_task_host(**fields)

    ret.update(
        dict(
            roles=['opensearch_cluster.masternode'],
            cluster_type='opensearch_cluster',
        )
    )

    return ret
