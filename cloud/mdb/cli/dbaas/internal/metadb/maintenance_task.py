from cloud.mdb.cli.dbaas.internal.db import db_query, db_transaction
from cloud.mdb.cli.dbaas.internal.metadb.common import to_db_cluster_types
from cloud.mdb.cli.dbaas.internal.metadb.exceptions import MaintenanceTaskNotFound
from cloud.mdb.cli.dbaas.internal.metadb.task import get_task, is_terminated, reject_task


def get_maintenance_task(ctx, task_id):
    query = """
        SELECT
            mt.cid "cluster_id",
            c.status "cluster_status",
            mt.config_id "type",
            mt.task_id,
            mt.create_ts,
            mt.plan_ts,
            mt.status,
            mt.info
        FROM dbaas.maintenance_tasks mt
        JOIN dbaas.clusters c USING (cid)
        WHERE mt.task_id = %(task_id)s;
        """

    result = db_query(ctx, 'metadb', query, task_id=task_id)

    if not result:
        raise MaintenanceTaskNotFound(task_id)

    return result[0]


def get_maintenance_tasks(
    ctx,
    *,
    cloud_id=None,
    cluster_ids=None,
    cluster_env=None,
    cluster_types=None,
    cluster_statuses=None,
    exclude_cluster_statuses=None,
    task_ids=None,
    type=None,
    statuses=None,
    limit=None
):
    query = """
        SELECT
            mt.cid "cluster_id",
            c.env "cluster_env",
            c.status "cluster_status",
            mt.config_id "type",
            mt.task_id,
            mt.create_ts,
            mt.plan_ts,
            mt.status,
            mt.info
        FROM dbaas.maintenance_tasks mt
        JOIN dbaas.clusters c USING (cid)
        JOIN dbaas.folders f USING (folder_id)
        JOIN dbaas.clouds cl USING (cloud_id)
        WHERE true
        {% if cloud_id %}
          AND cl.cloud_ext_id = %(cloud_id)s
        {% endif %}
        {% if cluster_ids %}
          AND mt.cid = ANY(%(cluster_ids)s)
        {% endif %}
        {% if cluster_env %}
          AND c.env::text = %(cluster_env)s
        {% endif %}
        {% if cluster_types %}
          AND c.type = ANY(%(cluster_types)s::dbaas.cluster_type[])
        {% endif %}
        {% if cluster_statuses %}
          AND c.status = ANY(%(cluster_statuses)s::dbaas.cluster_status[])
        {% endif %}
        {% if exclude_cluster_statuses %}
          AND c.status != ALL(%(exclude_cluster_statuses)s::dbaas.cluster_status[])
        {% endif %}
        {% if task_ids %}
          AND mt.task_id = ANY(%(task_ids)s)
        {% endif %}
        {% if type %}
          AND mt.config_id LIKE %(type)s
        {% endif %}
        {% if statuses %}
          AND mt.status::text = ANY(%(statuses)s)
        {% endif %}
        ORDER BY mt.plan_ts DESC
        {% if limit %}
        LIMIT {{ limit }}
        {% endif %}
        """
    return db_query(
        ctx,
        'metadb',
        query,
        cloud_id=cloud_id,
        cluster_ids=cluster_ids,
        cluster_env=cluster_env,
        cluster_types=to_db_cluster_types(cluster_types),
        cluster_statuses=cluster_statuses,
        exclude_cluster_statuses=exclude_cluster_statuses,
        task_ids=task_ids,
        type=type,
        statuses=statuses,
        limit=limit,
    )


def cancel_maintenance_task(ctx, task_id):
    with db_transaction(ctx, 'metadb'):
        task = get_task(ctx, task_id)
        if not is_terminated(task):
            reject_task(
                ctx,
                task_id,
                errors=[
                    {
                        "code": 1,
                        "type": "Cancelled",
                        "message": "The planned maintenance was canceled",
                        "exposable": True,
                    }
                ],
            )

        update_maintenance_task_status(ctx, task_id, status='CANCELED')


def update_maintenance_task_status(ctx, task_id, status):
    db_query(
        ctx,
        'metadb',
        """
             UPDATE dbaas.maintenance_tasks
             SET
                 status = %(status)s
             WHERE task_id = %(task_id)s;
             """,
        task_id=task_id,
        status=status,
        fetch=False,
    )


def update_maintenance_task_plan_ts(ctx, task_id, plan_ts):
    db_query(
        ctx,
        'metadb',
        """
             UPDATE dbaas.maintenance_tasks
             SET
                 plan_ts = %(plan_ts)s
             WHERE task_id = %(task_id)s;
             """,
        task_id=task_id,
        plan_ts=plan_ts,
        fetch=False,
    )
