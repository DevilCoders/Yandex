# -*- coding: utf-8 -*-
"""
DBaaS Internal API MySQL cluster delete
"""

from ...utils import pillar
from ...utils.cluster.delete import delete_cluster_tasks
from ...utils.register import DbaasOperation, Resource, register_request_handler
from .constants import MY_CLUSTER_TYPE
from .pillar import get_cluster_pillar
from .traits import MySQLOperations, MySQLTasks


@register_request_handler(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.DELETE)
def mysql_cluster_delete(cluster, undeleted_hosts, **_):
    """
    Delete MySQL cluster
    """
    options = cluster['value']

    task_args = {
        's3_buckets': {
            'backup': pillar.get_s3_bucket_for_delete(options),
        },
        'zk_hosts': get_cluster_pillar(cluster).zk_hosts,
    }

    if undeleted_hosts:
        task_args['hosts'] = undeleted_hosts

    return delete_cluster_tasks(
        cid=cluster['cid'],
        task_args=task_args,
        tasks_enum=MySQLTasks,
        operations_enum=MySQLOperations,
    )
