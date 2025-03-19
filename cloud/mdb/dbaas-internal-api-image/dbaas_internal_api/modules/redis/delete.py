"""
DBaaS Internal API Redis cluster delete
"""
from ...utils import pillar
from ...utils.cluster.delete import delete_cluster_tasks
from ...utils.feature_flags import ensure_no_feature_flag
from ...utils.register import DbaasOperation, Resource, register_request_handler
from .constants import MY_CLUSTER_TYPE
from .traits import RedisOperations, RedisTasks


@register_request_handler(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.DELETE)
def redis_cluster_delete(cluster, undeleted_hosts, **_):
    """
    Delete Redis cluster
    """
    ensure_no_feature_flag('MDB_REDIS_DISABLE_LEGACY_CLUSTER_CRUD_API')
    options = cluster['value']

    task_args = {
        's3_buckets': {
            'backup': pillar.get_s3_bucket_for_delete(options),
        },
    }

    if undeleted_hosts:
        task_args['hosts'] = undeleted_hosts

    return delete_cluster_tasks(
        cid=cluster['cid'],
        task_args=task_args,
        tasks_enum=RedisTasks,
        operations_enum=RedisOperations,
    )
