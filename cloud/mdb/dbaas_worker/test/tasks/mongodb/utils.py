"""
MongoDB task test utils
"""

from test.tasks.utils import get_compute_task_host, get_porto_task_host


def get_mongod_porto_host(**fields):
    """
    Get mongod porto host for task args
    """
    ret = get_porto_task_host(**fields)

    ret.update(dict(roles=['mongodb_cluster.mongod']))

    return ret


def get_mongocfg_porto_host(**fields):
    """
    Get mongocfg porto host for task args
    """
    ret = get_porto_task_host(**fields)

    ret.update(dict(roles=['mongodb_cluster.mongocfg']))

    return ret


def get_mongoinfra_porto_host(**fields):
    """
    Get mongoinfra porto host for task args
    """
    ret = get_porto_task_host(**fields)

    ret.update(dict(roles=['mongodb_cluster.mongoinfra']))

    return ret


def get_mongos_porto_host(**fields):
    """
    Get mongos porto host for task args
    """
    ret = get_porto_task_host(**fields)

    ret.update(dict(roles=['mongodb_cluster.mongos']))

    return ret


def get_mongod_compute_host(**fields):
    """
    Get mongod compute host for task args
    """
    ret = get_compute_task_host(**fields)

    ret.update(
        dict(
            roles=['mongodb_cluster.mongod'],
            cluster_type='mongodb_cluster',
        )
    )

    return ret


def get_mongocfg_compute_host(**fields):
    """
    Get mongocfg compute host for task args
    """
    ret = get_compute_task_host(**fields)

    ret.update(
        dict(
            roles=['mongodb_cluster.mongocfg'],
            cluster_type='mongodb_cluster',
        )
    )

    return ret


def get_mongos_compute_host(**fields):
    """
    Get mongos compute host for task args
    """
    ret = get_compute_task_host(**fields)

    ret.update(
        dict(
            roles=['mongodb_cluster.mongos'],
            cluster_type='mongodb_cluster',
        )
    )

    return ret


def get_mongoinfra_compute_host(**fields):
    """
    Get mongos compute host for task args
    """
    ret = get_compute_task_host(**fields)

    ret.update(
        dict(
            roles=['mongodb_cluster.mongoinfra'],
            cluster_type='mongodb_cluster',
        )
    )

    return ret
