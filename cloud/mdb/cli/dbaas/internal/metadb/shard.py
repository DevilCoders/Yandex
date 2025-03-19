from cloud.mdb.cli.dbaas.internal.db import db_query
from cloud.mdb.cli.dbaas.internal.metadb.common import to_db_cluster_types
from cloud.mdb.cli.dbaas.internal.metadb.exceptions import ShardNotFound
from cloud.mdb.cli.dbaas.internal.metadb.host import to_overlay_fqdn


def get_shard(ctx, *, untyped_id=None, shard_id=None):
    assert sum(bool(v) for v in (untyped_id, shard_id)) == 1

    if untyped_id:
        untyped_id = to_overlay_fqdn(ctx, untyped_id)

    result = db_query(
        ctx,
        'metadb',
        """
                      SELECT
                          s.shard_id "id",
                          s.name,
                          sc.roles,
                          sc.cid "cluster_id",
                          c.name "cluster_name",
                          c.status "cluster_status",
                          sc.subcid "subcluster_id",
                          s.created_at,
                          p.value "pillar"
                      FROM dbaas.shards s
                      JOIN dbaas.subclusters sc USING (subcid)
                      JOIN dbaas.clusters c USING (cid)
                      LEFT JOIN dbaas.pillar p USING (shard_id)
                      {% if shard_id %}
                      WHERE s.shard_id = %(shard_id)s
                      {% else %}
                      WHERE s.shard_id = %(untyped_id)s
                      OR EXISTS (SELECT 1
                                 FROM dbaas.hosts
                                 WHERE shard_id = s.shard_id AND fqdn = %(untyped_id)s)
                      {% endif %}
                      """,
        untyped_id=untyped_id,
        shard_id=shard_id,
    )

    if not result:
        raise ShardNotFound(shard_id)

    return result[0]


def get_shards(
    ctx,
    *,
    untyped_ids=None,
    folder_id=None,
    cluster_id=None,
    subcluster_id=None,
    subcluster_ids=None,
    shard_ids=None,
    exclude_shard_ids=None,
    hostnames=None,
    cluster_env=None,
    cluster_types=None,
    cluster_statuses=None,
    exclude_cluster_statuses=None,
    role=None,
    pillar_filter=None,
    limit=None
):
    if untyped_ids:
        untyped_ids = [to_overlay_fqdn(ctx, fqdn) for fqdn in untyped_ids]

    query = """
        SELECT
            s.shard_id "id",
            s.name,
            sc.roles,
            sc.cid "cluster_id",
            c.name "cluster_name",
            c.status "cluster_status",
            sc.subcid "subcluster_id",
            s.created_at,
            p.value "pillar"
        FROM dbaas.shards s
        JOIN dbaas.subclusters sc USING (subcid)
        JOIN dbaas.clusters c USING (cid)
        JOIN dbaas.folders f USING (folder_id)
        LEFT JOIN dbaas.pillar p USING (shard_id)
        WHERE true
        {% if folder_id %}
          AND f.folder_ext_id = %(folder_id)s
        {% endif %}
        {% if cluster_id %}
          AND sc.cid = %(cluster_id)s
        {% endif %}
        {% if subcluster_id %}
          AND sc.subcid = %(subcluster_id)s
        {% endif %}
        {% if subcluster_ids %}
          AND sc.subcid = ANY(%(subcluster_ids)s)
        {% endif %}
        {% if shard_ids %}
          AND s.shard_id = ANY(%(shard_ids)s)
        {% endif %}
        {% if exclude_shard_ids %}
          AND s.shard_id != ALL(%(exclude_shard_ids)s)
        {% endif %}
        {% if hostnames %}
          AND EXISTS (SELECT 1
                      FROM dbaas.hosts
                      WHERE shard_id = s.shard_id AND fqdn = ANY(%(hostnames)s))
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
        {% if role %}
          AND %(role)s = ANY(sc.roles::text[])
        {% endif %}
        {% if pillar_filter %}
          AND jsonb_path_match(p.value, %(pillar_filter)s)
        {% endif %}
        {% if untyped_ids %}
          AND (
              sc.cid = ANY(%(untyped_ids)s)
              OR sc.subcid = ANY(%(untyped_ids)s)
              OR s.shard_id = ANY(%(untyped_ids)s)
              OR EXISTS (SELECT 1
                         FROM dbaas.hosts
                         WHERE shard_id = s.shard_id AND fqdn = ANY(%(untyped_ids)s))
        )
        {% endif %}
        ORDER BY cluster_id, created_at
        {% if limit %}
        LIMIT {{ limit }}
        {% endif %}
        """

    shards = db_query(
        ctx,
        'metadb',
        query,
        untyped_ids=untyped_ids,
        folder_id=folder_id,
        cluster_id=cluster_id,
        subcluster_id=subcluster_id,
        subcluster_ids=subcluster_ids,
        shard_ids=shard_ids,
        exclude_shard_ids=exclude_shard_ids,
        hostnames=hostnames,
        cluster_env=cluster_env,
        cluster_types=to_db_cluster_types(cluster_types),
        cluster_statuses=cluster_statuses,
        exclude_cluster_statuses=exclude_cluster_statuses,
        role=role,
        pillar_filter=pillar_filter,
        limit=limit,
    )

    return shards


def update_shard_name(ctx, shard_id, new_name):
    db_query(
        ctx,
        'metadb',
        """
             UPDATE dbaas.shards
             SET
                 name = %(new_name)s
             WHERE shard_id = %(shard_id)s
             """,
        shard_id=shard_id,
        new_name=new_name,
        fetch=False,
    )
