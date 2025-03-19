"""
Redis task test utils
"""

from test.tasks.utils import get_compute_task_host, get_porto_task_host


def get_redis_porto_host(**fields):
    """
    Get redis porto host for task args
    """
    ret = get_porto_task_host(**fields)

    ret.update(
        dict(
            roles=['redis_cluster'],
            cluster_type='redis_cluster',
        )
    )

    return ret


def get_redis_compute_host(**fields):
    """
    Get redis compute host for task args
    """
    ret = get_compute_task_host(**fields)

    ret.update(
        dict(
            roles=['redis_cluster'],
            cluster_type='redis_cluster',
        )
    )

    return ret
