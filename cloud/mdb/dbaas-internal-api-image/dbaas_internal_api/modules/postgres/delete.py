# -*- coding: utf-8 -*-
"""
DBaaS Internal API postgresql cluster delete
"""
from ...utils import metadb
from ...utils.cluster.delete import delete_cluster_tasks
from ...utils.register import DbaasOperation, Resource, register_request_handler
from .constants import MY_CLUSTER_TYPE
from .pillar import PostgresqlClusterPillar
from .traits import PostgresqlOperations, PostgresqlTasks


@register_request_handler(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.DELETE)
def postgresql_cluster_delete(cluster, undeleted_hosts, **_):
    """
    Delete postgresql cluster
    """
    cid = cluster['cid']
    cluster_pillar = PostgresqlClusterPillar(cluster['value'])

    task_args = {
        'zk_hosts': ",".join(cluster_pillar.pgsync.get_zk_hosts()),
        's3_buckets': {
            'backup': cluster_pillar.s3_bucket,
        },
    }

    if undeleted_hosts:
        task_args['hosts'] = undeleted_hosts

    ag_ids = metadb.list_alert_groups(cid)

    if ag_ids:
        task_args['alert_groups'] = ag_ids
        for ag in ag_ids:
            metadb.delete_alert_group(
                alert_group_id=ag['alert_group_id'], cluster_id=cid, force_managed_group_deletion=True
            )

    return delete_cluster_tasks(
        cid=cid,
        task_args=task_args,
        tasks_enum=PostgresqlTasks,
        operations_enum=PostgresqlOperations,
    )
