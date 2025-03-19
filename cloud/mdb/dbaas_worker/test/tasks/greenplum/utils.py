"""
Greenplum task test utils
"""

from test.tasks.utils import get_compute_task_host, get_porto_task_host


def get_greenplum_master_porto_host(**fields):
    """
    Get greenplum porto host for task args
    """
    ret = get_porto_task_host(**fields)

    ret.update(dict(roles=['greenplum_cluster.master_subcluster']))

    return ret


def get_greenplum_segment_porto_host(**fields):
    """
    Get greenplum porto host for task args
    """
    ret = get_porto_task_host(**fields)

    ret.update(dict(roles=['greenplum_cluster.segment_subcluster']))

    return ret


def get_greenplum_master_compute_host(**fields):
    """
    Get greenplum compute host for task args
    """
    ret = get_compute_task_host(**fields)

    ret.update(
        dict(
            roles=['greenplum_cluster.master_subcluster'],
            cluster_type='greenplum_cluster',
        )
    )

    return ret


def get_greenplum_segment_compute_host(**fields):
    """
    Get greenplum compute host for task args
    """
    ret = get_compute_task_host(**fields)

    ret.update(
        dict(
            roles=['greenplum_cluster.segment_subcluster'],
            cluster_type='greenplum_cluster',
        )
    )

    return ret
