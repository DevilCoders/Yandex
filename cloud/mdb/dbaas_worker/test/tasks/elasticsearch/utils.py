"""
ElasticSearch task test utils
"""

from test.tasks.utils import get_compute_task_host, get_porto_task_host


def get_elasticsearch_datanode_porto_host(**fields):
    """
    Get elasticsearch datanode porto host for task args
    """
    ret = get_porto_task_host(**fields)

    ret.update(dict(roles=['elasticsearch_cluster.datanode']))

    return ret


def get_elasticsearch_masternode_porto_host(**fields):
    """
    Get elasticsearch masternode porto host for task args
    """
    ret = get_porto_task_host(**fields)

    ret.update(dict(roles=['elasticsearch_cluster.masternode']))

    return ret


def get_elasticsearch_datanode_compute_host(**fields):
    """
    Get elasticsearch datanode compute host for task args
    """
    ret = get_compute_task_host(**fields)

    ret.update(
        dict(
            roles=['elasticsearch_cluster.datanode'],
            cluster_type='elasticsearch_cluster',
        )
    )

    return ret


def get_elasticsearch_masternode_compute_host(**fields):
    """
    Get elasticsearch masternode compute host for task args
    """
    ret = get_compute_task_host(**fields)

    ret.update(
        dict(
            roles=['elasticsearch_cluster.masternode'],
            cluster_type='elasticsearch_cluster',
        )
    )

    return ret
