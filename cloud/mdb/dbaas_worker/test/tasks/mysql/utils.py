"""
MySQL task test utils
"""

from test.tasks.utils import get_compute_task_host, get_porto_task_host


def get_mysql_porto_host(**fields):
    """
    Get mysql porto host for task args
    """
    ret = get_porto_task_host(**fields)

    ret.update(dict(roles=['mysql_cluster']))

    return ret


def get_mysql_compute_host(**fields):
    """
    Get mysql compute host for task args
    """
    ret = get_compute_task_host(**fields)

    ret.update(
        dict(
            roles=['mysql_cluster'],
            cluster_type='mysql_cluster',
        )
    )

    return ret
