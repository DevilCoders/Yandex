import getpass
import json
import os

from click import ClickException

from cloud.mdb.cli.dbaas.internal.db import db_query
from cloud.mdb.cli.dbaas.internal.metadb.common import to_db_cluster_types


def _kdb_query(ctx, query, **kwargs):
    return db_query(ctx, "katandb", query, **kwargs)


def get_schedule(ctx, schedule_id, with_stats):
    result = _kdb_query(
        ctx,
        """
        SELECT
            schedule_id "id",
            namespace,
            name,
            schedules.state,
            match_tags,
            commands,
            age,
            still_age,
            max_size,
            parallel,
            options,
            edited_by,
            edited_at
        FROM katan.schedules
        WHERE schedules.schedule_id = %(schedule_id)s
        """,
        schedule_id=schedule_id,
    )

    if not result:
        raise ClickException(f'Schedule "{schedule_id}" not found.')

    row = dict(result[0])
    if with_stats:
        row["matched_clusters"] = _kdb_query(
            ctx,
            "SELECT count(*) FROM katan.clusters WHERE tags @> %(match_tags)s",
            match_tags=row["match_tags"],
        )[0][0]
        row["actual_rollouts_state"] = {}
        for stat in _kdb_query(
            ctx,
            """
                SELECT cluster_rollouts.state, count(*) count
                  FROM katan.rollouts
                  JOIN katan.cluster_rollouts USING (rollout_id)
                 WHERE rollouts.schedule_id = %(schedule_id)s
                   AND cluster_rollouts.updated_at >= (now() - %(age)s)
                 GROUP BY cluster_rollouts.state
                """,
            schedule_id=schedule_id,
            age=row["age"],
        ):
            row["actual_rollouts_state"][stat[0]] = stat[1]

    return row


def list_schedules(ctx, name, namespace, limit=None):
    return _kdb_query(
        ctx,
        """
        SELECT
            schedule_id "id",
            namespace,
            name,
            schedules.state,
            match_tags,
            commands,
            age,
            still_age,
            max_size,
            parallel,
            options,
            edited_by,
            edited_at
        FROM katan.schedules
        WHERE true
        {% if name %}
          AND name LIKE %(name)s
        {% endif %}
        {% if namespace %}
          AND namespace LIKE %(namespace)s
        {% endif %}
        ORDER BY namespace, name
        {% if limit %}
        LIMIT {{ limit }}
        {% endif %}
        """,
        name=name,
        namespace=namespace,
        limit=limit,
    )


def create_schedule(
    ctx, *, name, namespace, match_tags, commands, age, still_age, max_size, parallel, options=None, edited_by=None
):
    if edited_by is None:
        edited_by = os.environ.get('USER')

    if options is None:
        options = {}

    result = _kdb_query(
        ctx,
        """
        INSERT INTO katan.schedules (
            name,
            namespace,
            match_tags,
            commands,
            age,
            still_age,
            max_size,
            parallel,
            options,
            edited_by
        )
        VALUES (
            %(name)s,
            %(namespace)s,
            %(match_tags)s,
            %(commands)s,
            %(age)s,
            %(still_age)s,
            %(max_size)s,
            %(parallel)s,
            %(options)s,
            %(edited_by)s
        )
        RETURNING schedule_id "id"
        """,
        name=name,
        namespace=namespace,
        match_tags=json.dumps(match_tags),
        commands=json.dumps(commands),
        age=age,
        still_age=still_age,
        max_size=max_size,
        parallel=parallel,
        options=json.dumps(options),
        edited_by=edited_by,
    )

    return result[0]['id']


def update_schedule(
    ctx,
    schedule_id,
    *,
    name=None,
    namespace=None,
    match_tags=None,
    commands=None,
    age=None,
    still_age=None,
    max_size=None,
    parallel=None,
    options=None,
    edited_by=None,
):
    if edited_by is None:
        edited_by = os.environ.get('USER')

    if match_tags is not None:
        match_tags = json.dumps(match_tags)

    if commands is not None:
        commands = json.dumps(commands)

    if options is not None:
        options = json.dumps(options)

    result = _kdb_query(
        ctx,
        """
        UPDATE katan.schedules
        SET
        {% if name %}
            name = %(name)s,
        {% endif %}
        {% if namespace %}
            namespace = %(namespace)s,
        {% endif %}
        {% if match_tags %}
            match_tags = %(match_tags)s,
        {% endif %}
        {% if commands %}
            commands = %(commands)s,
        {% endif %}
        {% if age %}
            age = %(age)s,
        {% endif %}
        {% if still_age %}
            still_age = %(still_age)s,
        {% endif %}
        {% if max_size %}
            max_size = %(max_size)s,
        {% endif %}
        {% if parallel %}
            parallel = %(parallel)s,
        {% endif %}
        {% if options %}
            options = %(options)s,
        {% endif %}
            edited_by = %(edited_by)s
        WHERE schedule_id = %(schedule_id)s
        RETURNING schedule_id "id"
        """,
        schedule_id=schedule_id,
        name=name,
        namespace=namespace,
        match_tags=match_tags,
        commands=commands,
        age=age,
        still_age=still_age,
        max_size=max_size,
        parallel=parallel,
        options=options,
        edited_by=edited_by,
    )

    if not result:
        raise ClickException(f'Schedule "{schedule_id}" not found.')


def activate_schedule(ctx, schedule_id):
    return _change_schedule_state(ctx, schedule_id, "active")


def stop_schedule(ctx, schedule_id):
    return _change_schedule_state(ctx, schedule_id, "stopped")


def delete_schedule(ctx, schedule_id):
    result = _kdb_query(
        ctx,
        """
                        DELETE FROM katan.schedules
                        WHERE schedule_id = %(schedule_id)s
                        RETURNING schedule_id
                        """,
        schedule_id=schedule_id,
    )

    if not result:
        raise ClickException(f'Schedule "{schedule_id}" not found.')


def list_clusters(ctx, *, cluster_ids=None, cluster_types=None, limit=None):
    return _kdb_query(
        ctx,
        """
        SELECT
            cluster_id "id",
            tags,
            imported_at,
            auto_update
        FROM katan.clusters
        WHERE true
        {% if cluster_ids %}
          AND cluster_id = ANY(%(cluster_ids)s)
        {% endif %}
        {% if cluster_types %}
          AND tags#>>'{meta,type}' = ANY(%(cluster_types)s)
        {% endif %}
        ORDER BY imported_at DESC
        {% if limit %}
        LIMIT {{ limit }}
        {% endif %}
        """,
        cluster_ids=cluster_ids,
        cluster_types=to_db_cluster_types(cluster_types),
        limit=limit,
    )


def update_cluster(ctx, cluster_id, *, auto_update=None):
    result = _kdb_query(
        ctx,
        """
        UPDATE katan.clusters
        SET
        {% if auto_update is not none %}
            auto_update = %(auto_update)s,
        {% endif %}
            cluster_id = %(cluster_id)s
        WHERE cluster_id = %(cluster_id)s
        RETURNING cluster_id "id"
        """,
        cluster_id=cluster_id,
        auto_update=auto_update,
    )

    if not result:
        raise ClickException(f'Cluster "{cluster_id}" not found.')


def list_rollouts(ctx, schedule_id, limit=None):
    return _kdb_query(
        ctx,
        """
        SELECT
            r.rollout_id "id",
            r.commands,
            r.parallel,
            r.created_at,
            r.started_at,
            r.finished_at,
            r.created_by,
            r.schedule_id,
            s.name "schedule_name",
            r.comment,
            r.rolled_by,
            r.options
        FROM katan.rollouts r
        JOIN katan.schedules s USING (schedule_id)
        WHERE true
        {% if schedule_id %}
          AND schedule_id = %(schedule_id)s
        {% endif %}
        ORDER BY created_at DESC
        {% if limit %}
        LIMIT {{ limit }}
        {% endif %}
        """,
        schedule_id=schedule_id,
        limit=limit,
    )


def list_cluster_rollouts(
    ctx, *, schedule_id=None, rollout_id=None, cluster_ids=None, cluster_types=None, state=None, limit=None
):
    return _kdb_query(
        ctx,
        """
        SELECT
            cr.rollout_id,
            cr.cluster_id,
            cr.state,
            cr.updated_at,
            cr.comment
        FROM katan.cluster_rollouts cr
        JOIN katan.rollouts r USING (rollout_id)
        JOIN katan.clusters c USING (cluster_id)
        WHERE true
        {% if schedule_id %}
          AND r.schedule_id = %(schedule_id)s
        {% endif %}
        {% if rollout_id %}
          AND cr.rollout_id = %(rollout_id)s
        {% endif %}
        {% if cluster_ids %}
          AND cr.cluster_id = ANY(%(cluster_ids)s)
        {% endif %}
        {% if cluster_types %}
          AND c.tags#>>'{meta,type}' = ANY(%(cluster_types)s)
        {% endif %}
        {% if state %}
          AND cr.state::text = %(state)s
        {% endif %}
        ORDER BY updated_at DESC
        {% if limit %}
        LIMIT {{ limit }}
        {% endif %}
        """,
        schedule_id=schedule_id,
        rollout_id=rollout_id,
        cluster_ids=cluster_ids,
        cluster_types=to_db_cluster_types(cluster_types),
        state=state,
        limit=limit,
    )


def _change_schedule_state(ctx, schedule_id, state):
    result = _kdb_query(
        ctx,
        """
        UPDATE katan.schedules
           SET state = %(state)s,
               edited_by = %(user)s,
               edited_at = now()
         WHERE schedule_id = %(schedule_id)s
           AND state != %(state)s
        RETURNING schedule_id,
                  namespace,
                  name,
                  state,
                  match_tags,
                  commands,
                  age
        """,
        schedule_id=schedule_id,
        state=state,
        user=getpass.getuser(),
    )

    if not result:
        raise ClickException(f'Schedule "{schedule_id}" not found or already {state}.')

    return result[0]
