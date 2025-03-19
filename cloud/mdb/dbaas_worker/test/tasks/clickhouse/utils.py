"""
ClickHouse task test utils
"""

from test.tasks.utils import get_compute_task_host, get_porto_task_host


def get_clickhouse_porto_host(**fields):
    """
    Get clickhouse porto host for task args
    """
    ret = get_porto_task_host(**fields)

    ret.update(
        dict(
            roles=['clickhouse_cluster'],
            cluster_type='clickhouse_cluster',
        )
    )

    return ret


def get_zookeeper_porto_host(**fields):
    """
    Get zookeeper porto host for task args
    """
    ret = get_porto_task_host(**fields)

    ret.update(
        dict(
            roles=['zk'],
            cluster_type='zk',
        )
    )

    return ret


def get_clickhouse_compute_host(**fields):
    """
    Get clickhouse compute host for task args
    """
    ret = get_compute_task_host(**fields)

    ret.update(
        dict(
            roles=['clickhouse_cluster'],
            cluster_type='clickhouse_cluster',
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
