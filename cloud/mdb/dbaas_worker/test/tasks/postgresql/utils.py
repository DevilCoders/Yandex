"""
PostgreSQL task test utils
"""

from test.tasks.utils import get_compute_task_host, get_porto_task_host


def get_postgresql_porto_host(**fields):
    """
    Get postgresql porto host for task args
    """
    ret = get_porto_task_host(**fields)

    ret.update(dict(roles=['postgresql_cluster']))

    return ret


def get_postgresql_compute_host(**fields):
    """
    Get postgresql compute host for task args
    """
    ret = get_compute_task_host(**fields)

    ret.update(
        dict(
            roles=['postgresql_cluster'],
            cluster_type='postgresql_cluster',
        )
    )

    return ret
