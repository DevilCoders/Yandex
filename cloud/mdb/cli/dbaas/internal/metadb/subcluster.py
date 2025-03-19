from cloud.mdb.cli.dbaas.internal.metadb.common import to_db_cluster_types
from cloud.mdb.cli.dbaas.internal.db import db_query, MultipleRecordsError
from cloud.mdb.cli.dbaas.internal.metadb.exceptions import SubclusterNotFound
from cloud.mdb.cli.dbaas.internal.metadb.host import to_overlay_fqdn


def get_subcluster(ctx, subcluster_id=None, *, untyped_id=None, cluster_id=None, role=None, with_stats=False):
    """
    Get subcluster from metadb.
    """
    assert sum(bool(v) for v in (subcluster_id, untyped_id, cluster_id)) == 1

    subclusters = get_subclusters(
        ctx,
        subcluster_ids=[subcluster_id] if subcluster_id else None,
        untyped_ids=[untyped_id] if untyped_id else None,
        cluster_id=cluster_id,
        role=role,
        with_stats=with_stats,
    )

    if not subclusters:
        raise SubclusterNotFound(subcluster_id)

    if len(subclusters) > 1:
        raise MultipleRecordsError()

    return subclusters[0]


def get_subclusters(
    ctx,
    untyped_ids=None,
    folder_id=None,
    cluster_id=None,
    cluster_ids=None,
    exclude_cluster_ids=None,
    subcluster_ids=None,
    exclude_subcluster_ids=None,
    hostnames=None,
    cluster_env=None,
    cluster_types=None,
    cluster_statuses=None,
    exclude_cluster_statuses=None,
    role=None,
    flavor_type=None,
    exclude_flavor_type=None,
    min_host_count=None,
    max_host_count=None,
    pillar_filter=None,
    with_stats=False,
    limit=None,
):
    """
    Get subclusters from metadb.
    """
    if hostnames:
        hostnames = [to_overlay_fqdn(ctx, fqdn) for fqdn in hostnames]

    if untyped_ids:
        untyped_ids = [to_overlay_fqdn(ctx, fqdn) for fqdn in untyped_ids]

    query = """
        {% if with_stats %}
        WITH subcluster_resources AS (
            SELECT
                sc.subcid,
                count(1) hosts,
                sum(f.cpu_guarantee) cpu_cores,
                sum(f.memory_guarantee) memory_size,
                sum(h.space_limit) disk_size
            FROM dbaas.subclusters sc
            LEFT JOIN dbaas.hosts h USING (subcid)
            JOIN dbaas.flavors f ON (f.id = h.flavor)
            GROUP BY sc.subcid
        )
        {% endif %}
        SELECT
            sc.subcid "id",
            sc.name,
            sc.roles,
            sc.cid "cluster_id",
            c.name "cluster_name",
            sc.created_at,
        {% if with_stats %}
            scr.hosts,
            scr.cpu_cores,
            scr.memory_size,
            scr.disk_size,
        {% endif %}
            p.value "pillar"
        FROM
            dbaas.subclusters sc
            JOIN dbaas.clusters c USING (cid)
            JOIN dbaas.folders f USING (folder_id)
        {% if with_stats %}
            JOIN subcluster_resources scr USING (subcid)
        {% endif %}
            LEFT JOIN dbaas.pillar p USING (subcid)
        WHERE true
        {% if folder_id %}
          AND f.folder_ext_id = %(folder_id)s
        {% endif %}
        {% if cluster_id %}
          AND c.cid = %(cluster_id)s
        {% endif %}
        {% if cluster_ids %}
          AND c.cid = ANY(%(cluster_ids)s)
        {% endif %}
        {% if exclude_cluster_ids %}
          AND c.cid != ALL(%(exclude_cluster_ids)s)
        {% endif %}
        {% if subcluster_ids %}
          AND sc.subcid = ANY(%(subcluster_ids)s)
        {% endif %}
        {% if exclude_subcluster_ids %}
          AND sc.subcid != ALL(%(exclude_subcluster_ids)s)
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
        {% if hostnames %}
          AND EXISTS (SELECT 1
                      FROM dbaas.hosts
                      WHERE subcid = sc.subcid AND fqdn = ANY(%(hostnames)s))
        {% endif %}
        {% if flavor_type %}
          AND EXISTS (SELECT 1
                      FROM dbaas.hosts
                      JOIN dbaas.flavors ON (flavors.id = hosts.flavor)
                      WHERE subcid = sc.subcid AND flavors.type = %(flavor_type)s)
        {% endif %}
        {% if exclude_flavor_type %}
          AND NOT EXISTS (SELECT 1
                          FROM dbaas.hosts
                          JOIN dbaas.flavors ON (flavors.id = hosts.flavor)
                          WHERE subcid = sc.subcid AND flavors.type = %(exclude_flavor_type)s)
        {% endif %}
        {% if min_host_count %}
          AND (SELECT count(*)
               FROM dbaas.hosts
               WHERE subcid = sc.subcid) >= %(min_host_count)s
        {% endif %}
        {% if max_host_count %}
          AND (SELECT count(*)
               FROM dbaas.hosts
               WHERE subcid = sc.subcid) <= %(max_host_count)s
        {% endif %}
        {% if untyped_ids %}
          AND (
              c.cid = ANY(%(untyped_ids)s)
              OR sc.subcid = ANY(%(untyped_ids)s)
              OR EXISTS (SELECT 1
                         FROM dbaas.shards
                         WHERE subcid = sc.subcid AND shard_id = ANY(%(untyped_ids)s))
              OR EXISTS (SELECT 1
                         FROM dbaas.hosts
                         WHERE subcid = sc.subcid AND fqdn = ANY(%(untyped_ids)s))
        )
        {% endif %}
        ORDER BY sc.created_at DESC
        {% if limit %}
        LIMIT {{ limit }}
        {% endif %}
        """
    subclusters = db_query(
        ctx,
        'metadb',
        query,
        untyped_ids=untyped_ids,
        folder_id=folder_id,
        cluster_id=cluster_id,
        cluster_ids=cluster_ids,
        subcluster_ids=subcluster_ids,
        hostnames=hostnames,
        cluster_env=cluster_env,
        cluster_types=to_db_cluster_types(cluster_types),
        cluster_statuses=cluster_statuses,
        exclude_cluster_statuses=exclude_cluster_statuses,
        role=role,
        exclude_cluster_ids=exclude_cluster_ids,
        exclude_subcluster_ids=exclude_subcluster_ids,
        flavor_type=flavor_type,
        exclude_flavor_type=exclude_flavor_type,
        min_host_count=min_host_count,
        max_host_count=max_host_count,
        pillar_filter=pillar_filter,
        with_stats=with_stats,
        limit=limit,
    )

    return subclusters


def delete_subcluster(ctx, cluster_id, subcluster_id, revision):
    """
    Delete subcluster from metadb.
    """
    db_query(
        ctx,
        'metadb',
        """
             SELECT code.delete_subcluster(
                i_subcid => %(subcluster_id)s,
                i_cid    => %(cluster_id)s,
                i_rev    => %(rev)s
             );
             """,
        cluster_id=cluster_id,
        subcluster_id=subcluster_id,
        rev=revision,
    )
