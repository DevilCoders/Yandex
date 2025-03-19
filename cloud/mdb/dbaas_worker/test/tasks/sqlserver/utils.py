"""
SQLServer task test utils
"""

from test.tasks.utils import get_compute_task_host


def get_sqlserver_compute_host(**fields):
    """
    Get sqlserver compute host for task args
    """
    ret = get_compute_task_host(**fields)

    ret.update(
        dict(
            roles=['sqlserver_cluster'],
            cluster_type='sqlserver_cluster',
        )
    )

    return ret


def get_witness_compute_host(**fields):
    """
    Get sqlserver compute host for task args
    """
    ret = get_compute_task_host(**fields)

    ret.update(
        dict(
            roles=['windows_witness'],
            cluster_type='sqlserver_cluster',
        )
    )

    return ret
