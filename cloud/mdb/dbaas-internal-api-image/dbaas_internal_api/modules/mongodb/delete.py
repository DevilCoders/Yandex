# -*- coding: utf-8 -*-
"""
DBaaS Internal API MongoDB cluster delete
"""
from ...utils import pillar
from ...utils.cluster.delete import delete_cluster_tasks
from ...utils.register import DbaasOperation, Resource, register_request_handler
from ...utils.worker import format_zk_hosts
from .constants import MY_CLUSTER_TYPE
from .traits import MongoDBOperations, MongoDBTasks


@register_request_handler(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.DELETE)
def mongodb_cluster_delete(cluster, undeleted_hosts, **_):
    """
    Delete MongoDB cluster
    """
    options = cluster['value']
    zk_hosts = format_zk_hosts(options['data']['mongodb']['zk_hosts'])

    task_args = {
        'zk_hosts': zk_hosts,
        's3_buckets': {
            'backup': pillar.get_s3_bucket_for_delete(options),
        },
    }

    if undeleted_hosts:
        task_args['hosts'] = undeleted_hosts

    return delete_cluster_tasks(
        cid=cluster['cid'],
        task_args=task_args,
        tasks_enum=MongoDBTasks,
        operations_enum=MongoDBOperations,
    )
