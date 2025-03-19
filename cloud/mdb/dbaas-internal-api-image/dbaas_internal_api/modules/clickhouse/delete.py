# -*- coding: utf-8 -*-
"""
DBaaS Internal API clickhouse cluster delete
"""
from ...utils.cluster.delete import delete_cluster_tasks
from ...utils.cluster.get import get_subcluster
from ...utils.feature_flags import ensure_no_feature_flag
from ...utils.pillar import get_s3_bucket_for_delete
from ...utils.register import DbaasOperation, Resource, register_request_handler
from .constants import MY_CLUSTER_TYPE
from .pillar import ClickhousePillar
from .traits import ClickhouseOperations, ClickhouseRoles, ClickhouseTasks
from .utils import get_zk_hosts_task_arg


@register_request_handler(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.DELETE)
def clickhouse_cluster_delete(cluster, subclusters, hosts, undeleted_hosts, **_):
    """
    Delete clickhouse cluster
    """
    ensure_no_feature_flag('MDB_CLICKHOUSE_DISABLE_LEGACY_DELETE_CLUSTER_API')

    subcluster = get_subcluster(subclusters=subclusters, role=ClickhouseRoles.clickhouse)
    pillar = ClickhousePillar.load(subcluster['value'])

    s3_buckets = {'backup': get_s3_bucket_for_delete(cluster['value'])}
    if pillar.cloud_storage_bucket:
        s3_buckets['cloud_storage'] = pillar.cloud_storage_bucket

    task_args = {
        'zk_hosts': get_zk_hosts_task_arg(hosts=hosts, pillar=pillar) if not pillar.embedded_keeper else [],
        's3_buckets': s3_buckets,
    }

    if undeleted_hosts:
        task_args['hosts'] = undeleted_hosts

    return delete_cluster_tasks(
        cid=cluster['cid'],
        task_args=task_args,
        tasks_enum=ClickhouseTasks,
        operations_enum=ClickhouseOperations,
    )
