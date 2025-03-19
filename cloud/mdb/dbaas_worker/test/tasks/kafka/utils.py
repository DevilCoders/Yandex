"""
Kafka task test utils
"""

from test.tasks.utils import get_compute_task_host, get_porto_task_host


def get_kafka_porto_host(**fields):
    """
    Get kafka porto host for task args
    """
    ret = get_porto_task_host(**fields)

    ret.update(dict(roles=['kafka_cluster']))

    return ret


def get_zookeeper_porto_host(**fields):
    """
    Get zookeeper porto host for task args
    """
    ret = get_porto_task_host(**fields)

    ret.update(dict(roles=['zk']))

    return ret


def get_kafka_compute_host(**fields):
    """
    Get kafka compute host for task args
    """
    ret = get_compute_task_host(**fields)

    ret.update(
        dict(
            roles=['kafka_cluster'],
            cluster_type='kafka_cluster',
        )
    )

    return ret


def get_zookeeper_compute_host(**fields):
    """
    Get zookeeper compute host for task args
    """
    ret = get_compute_task_host(**fields)

    ret.update(
        dict(
            roles=['zk'],
            cluster_type='zk',
        )
    )

    return ret
