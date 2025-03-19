from contextlib import contextmanager

from click import ClickException
from cloud.mdb.cli.dbaas.internal.db import db_query, db_transaction, MultipleRecordsError
from cloud.mdb.cli.dbaas.internal.metadb.common import to_cluster_type, to_db_cluster_type, to_db_cluster_types
from cloud.mdb.cli.dbaas.internal.metadb.exceptions import (
    ClusterNotFound,
    HostNotFound,
    ShardNotFound,
    SubclusterNotFound,
)
from cloud.mdb.cli.dbaas.internal.metadb.host import get_hosts, to_overlay_fqdn


def get_cluster(ctx, cluster_id=None, *, untyped_id=None, subcluster_id=None, shard_id=None, host=None):
    assert sum(bool(v) for v in (cluster_id, untyped_id, subcluster_id, shard_id, host)) == 1

    clusters = get_clusters(
        ctx,
        cluster_id=cluster_id,
        untyped_ids=[untyped_id] if untyped_id else None,
        subcluster_ids=[subcluster_id] if subcluster_id else None,
        shard_ids=[shard_id] if shard_id else None,
        hostnames=[host] if host else None,
    )

    if not clusters:
        if cluster_id:
            raise ClusterNotFound(cluster_id)
        elif subcluster_id:
            raise SubclusterNotFound(subcluster_id)
        elif shard_id:
            raise ShardNotFound(shard_id)
        elif host:
            raise HostNotFound(host)
        else:
            raise ClusterNotFound()

    if len(clusters) > 1:
        raise MultipleRecordsError()

    return clusters[0]


def get_clusters(
    ctx,
    *,
    untyped_ids=None,
    name=None,
    cloud_id=None,
    folder_ids=None,
    cluster_id=None,
    cluster_ids=None,
    subcluster_ids=None,
    exclude_cluster_ids=None,
    shard_ids=None,
    hostnames=None,
    vtype_ids=None,
    task_ids=None,
    cluster_type=None,
    cluster_types=None,
    version=None,
    pillar_filter=None,
    subcluster_pillar_filter=None,
    env=None,
    statuses=None,
    exclude_statuses=None,
    with_stats=False,
    order_by=None,
    limit=None,
):
    if hostnames:
        hostnames = [to_overlay_fqdn(ctx, fqdn) for fqdn in hostnames]

    if untyped_ids:
        untyped_ids = [to_overlay_fqdn(ctx, fqdn) for fqdn in untyped_ids]

    query = """
        WITH clusters AS (
            SELECT
                c.cid,
                c.name,
                c.type,
                CASE WHEN c.type = 'clickhouse_cluster'
                     THEN (SELECT value#>>'{data,clickhouse,ch_version}'
                           FROM dbaas.pillar JOIN dbaas.subclusters USING (subcid)
                           WHERE subclusters.cid = c.cid AND 'clickhouse_cluster' = ANY(subclusters.roles))
                     WHEN c.type = 'postgresql_cluster'
                     THEN (SELECT minor_version from dbaas.versions where cid = c.cid and component = 'postgres')
                     WHEN c.type = 'greenplum_cluster'
                     THEN (SELECT minor_version from dbaas.versions where cid = c.cid and component = 'greenplum')
                     WHEN c.type = 'mongodb_cluster'
                     THEN p.value#>>'{data,mongodb,version,major_human}'
                     WHEN c.type = 'redis_cluster'
                     THEN (SELECT minor_version from dbaas.versions where cid = c.cid and component = 'redis')
                     WHEN c.type = 'mysql_cluster'
                     THEN p.value#>>'{data,mysql,version,major_human}'
                     WHEN c.type = 'hadoop_cluster'
                     THEN p.value#>>'{data,unmanaged,version}'
                     WHEN c.type = 'kafka_cluster'
                     THEN p.value#>>'{data,kafka,version}'
                     WHEN c.type = 'elasticsearch_cluster'
                     THEN p.value#>>'{data,elasticsearch,version}'
                     WHEN c.type = 'opensearch_cluster'
                     THEN p.value#>>'{data,opensearch,version}'
                END "version",
                c.env,
                c.created_at,
                c.status,
                code.rev(c) revision,
                c.description,
                f.folder_ext_id,
                cl.cloud_ext_id,
                c.network_id,
                p.value "pillar"
            FROM
                dbaas.clusters c
                JOIN dbaas.folders f USING (folder_id)
                JOIN dbaas.clouds cl USING (cloud_id)
                LEFT JOIN dbaas.pillar p USING (cid)
            WHERE true
            {% if cloud_id %}
              AND cl.cloud_ext_id = %(cloud_id)s
            {% endif %}
            {% if folder_ids %}
              AND f.folder_ext_id = ANY(%(folder_ids)s)
            {% endif %}
            {% if cluster_id %}
              AND c.cid = %(cluster_id)s
            {% endif %}
            {% if cluster_ids %}
              AND c.cid = ANY(%(cluster_ids)s)
            {% endif %}
            {% if name %}
              AND c.name LIKE %(name)s
            {% endif %}
            {% if exclude_cluster_ids %}
              AND c.cid != ALL(%(exclude_cluster_ids)s)
            {% endif %}
            {% if cluster_type %}
              AND c.type = %(cluster_type)s::dbaas.cluster_type
            {% endif %}
            {% if cluster_types %}
              AND c.type = ANY(%(cluster_types)s::dbaas.cluster_type[])
            {% endif %}
            {% if env %}
              AND c.env::text = %(env)s
            {% endif %}
            {% if statuses %}
              AND c.status = ANY(%(statuses)s::dbaas.cluster_status[])
            {% endif %}
            {% if exclude_statuses %}
              AND c.status != ALL(%(exclude_statuses)s::dbaas.cluster_status[])
            {% endif %}
            {% if pillar_filter %}
              AND jsonb_path_match(p.value, %(pillar_filter)s)
            {% endif %}
            {% if subcluster_pillar_filter %}
                AND true = ANY((SELECT jsonb_path_match(subpillar.value, %(subcluster_pillar_filter)s)
                           FROM dbaas.pillar subpillar JOIN dbaas.subclusters USING (subcid)
                           WHERE subclusters.cid = c.cid))
            {% endif %}
            {% if subcluster_ids %}
              AND EXISTS (SELECT 1
                          FROM dbaas.subclusters
                          WHERE cid = c.cid AND subcid = ANY(%(subcluster_ids)s))
            {% endif %}
            {% if shard_ids %}
              AND EXISTS (SELECT 1
                          FROM dbaas.shards
                          LEFT JOIN dbaas.subclusters USING (subcid)
                          WHERE shard_id = ANY(%(shard_ids)s))
            {% endif %}
            {% if hostnames %}
              AND EXISTS (SELECT 1
                          FROM dbaas.hosts
                          JOIN dbaas.subclusters USING (subcid)
                          WHERE cid = c.cid AND fqdn = ANY(%(hostnames)s))
            {% endif %}
            {% if vtype_ids %}
              AND EXISTS (SELECT 1
                          FROM dbaas.hosts
                          JOIN dbaas.subclusters USING (subcid)
                          WHERE cid = c.cid AND vtype_id = ANY(%(vtype_ids)s))
            {% endif %}
            {% if task_ids %}
              AND EXISTS (SELECT 1
                          FROM dbaas.worker_queue
                          WHERE cid = c.cid AND task_id = ANY(%(task_ids)s))
            {% endif %}
            {% if untyped_ids %}
              AND (
                  c.cid = ANY(%(untyped_ids)s)
                  OR EXISTS (SELECT 1
                             FROM dbaas.subclusters
                             WHERE cid = c.cid AND subcid = ANY(%(untyped_ids)s))
                  OR EXISTS (SELECT 1
                             FROM dbaas.shards
                             LEFT JOIN dbaas.subclusters USING (subcid)
                             WHERE cid = c.cid AND shard_id = ANY(%(untyped_ids)s))
                  OR EXISTS (SELECT 1
                             FROM dbaas.hosts
                             JOIN dbaas.subclusters USING (subcid)
                             WHERE cid = c.cid AND fqdn = ANY(%(untyped_ids)s))
                  OR EXISTS (SELECT 1
                             FROM dbaas.worker_queue
                             WHERE cid = c.cid AND task_id = ANY(%(untyped_ids)s))
            )
            {% endif %}
        )
        {% if with_stats %}
        , cluster_resources AS (
            SELECT
                c.cid,
                count(1) hosts,
                sum(f.cpu_guarantee) cpu_cores,
                sum(f.memory_guarantee) memory_size,
                sum(h.space_limit) disk_size
            FROM clusters c
            JOIN dbaas.subclusters sc USING (cid)
            JOIN dbaas.hosts h USING (subcid)
            JOIN dbaas.flavors f ON (f.id = h.flavor)
            GROUP BY c.cid
        )
        {% endif %}
        SELECT
            c.cid "id",
            c.name,
            c.type,
            c.version,
            c.env,
            c.created_at,
            c.status,
            c.revision,
            c.description,
            c.folder_ext_id "folder_id",
            c.cloud_ext_id "cloud_id",
            c.network_id,
        {% if with_stats %}
            cr.hosts,
            cr.cpu_cores,
            cr.memory_size,
            cr.disk_size,
        {% endif %}
            c.pillar
        FROM clusters c
        {% if with_stats %}
        LEFT JOIN cluster_resources cr USING (cid)
        {% endif %}
        {% if version %}
        WHERE version LIKE (%(version)s || '%%')
        {% endif %}
        {% if order_by %}
        ORDER BY {{ order_by }}
        {% endif %}
        {% if limit %}
        LIMIT {{ limit }}
        {% endif %}
        """
    clusters = db_query(
        ctx,
        'metadb',
        query,
        untyped_ids=untyped_ids,
        name=name,
        cloud_id=cloud_id,
        folder_ids=folder_ids,
        cluster_id=cluster_id,
        cluster_ids=cluster_ids,
        subcluster_ids=subcluster_ids,
        exclude_cluster_ids=exclude_cluster_ids,
        shard_ids=shard_ids,
        hostnames=hostnames,
        vtype_ids=vtype_ids,
        task_ids=task_ids,
        cluster_type=to_db_cluster_type(cluster_type),
        cluster_types=to_db_cluster_types(cluster_types),
        env=env,
        version=version,
        statuses=statuses,
        exclude_statuses=exclude_statuses,
        pillar_filter=pillar_filter,
        subcluster_pillar_filter=subcluster_pillar_filter,
        with_stats=with_stats,
        order_by=order_by,
        limit=limit,
    )

    return clusters


def get_cluster_stats(ctx, *, cluster_type=None, role=None, zone=None):
    query = """
        WITH clusters AS (
            SELECT
                c.cid,
                c.type,
                CASE WHEN c.type = 'clickhouse_cluster'
                     THEN (SELECT value#>>'{data,clickhouse,ch_version}'
                           FROM dbaas.pillar JOIN dbaas.subclusters USING (subcid)
                           WHERE subclusters.cid = c.cid AND 'clickhouse_cluster' = ANY(subclusters.roles))
                     WHEN c.type = 'postgresql_cluster'
                     THEN (SELECT minor_version from dbaas.versions where cid = c.cid and component = 'postgres')
                     WHEN c.type = 'mongodb_cluster'
                     THEN p.value#>>'{data,mongodb,version,major_human}'
                     WHEN c.type = 'redis_cluster'
                     THEN (SELECT minor_version from dbaas.versions where cid = c.cid and component = 'redis')
                     WHEN c.type = 'mysql_cluster'
                     THEN p.value#>>'{data,mysql,version,major_human}'
                     WHEN c.type = 'hadoop_cluster'
                     THEN p.value#>>'{data,unmanaged,version}'
                     WHEN c.type = 'kafka_cluster'
                     THEN p.value#>>'{data,kafka,version}'
                     WHEN c.type = 'elasticsearch_cluster'
                     THEN p.value#>>'{data,elasticsearch,version}'
                     WHEN c.type = 'opensearch_cluster'
                     THEN p.value#>>'{data,opensearch,version}'
                END "version",
                count(1) hosts,
                sum(f.cpu_guarantee) cpu_cores,
                sum(f.memory_guarantee) memory_size,
                sum(h.space_limit) disk_size
            FROM dbaas.clusters c
            JOIN dbaas.subclusters sc USING (cid)
            JOIN dbaas.hosts h USING (subcid)
            JOIN dbaas.flavors f ON (f.id = h.flavor)
            JOIN dbaas.geo g USING (geo_id)
            LEFT JOIN dbaas.pillar p USING (cid)
            WHERE c.status::text != ALL('{STOPPED,DELETED,METADATA-DELETED,METADATA-DELETING,PURGED,PURGING}'::text[])
        {% if role %}
              AND %(role)s = ANY(sc.roles::text[])
        {% endif %}
        {% if cluster_type %}
              AND c.type = %(cluster_type)s::dbaas.cluster_type
        {% endif %}
        {% if zone %}
              AND g.name = %(zone)s
        {% endif %}
            GROUP BY c.cid, c.type, version
        )
        SELECT
            c.type,
            c.version,
            count(1) clusters,
            sum(c.hosts) hosts,
            sum(c.cpu_cores) cpu_cores,
            sum(c.memory_size) memory_size,
            round(sum(c.memory_size) / sum(c.cpu_cores)) memory_cpu_ration,
            sum(c.disk_size) disk_size
        FROM clusters c
        GROUP BY ROLLUP (c.type, c.version)
        ORDER BY c.type, c.version
        """
    return db_query(ctx, 'metadb', query, cluster_type=to_db_cluster_type(cluster_type), role=role, zone=zone)


def lock_cluster(ctx, cluster_id):
    result = db_query(
        ctx,
        'metadb',
        """
                      SELECT
                          cid "cluster_id",
                          rev
                      FROM code.lock_cluster(%(cluster_id)s)
                      """,
        cluster_id=cluster_id,
    )

    if not result:
        raise ClusterNotFound(cluster_id)

    ctx.obj['cluster_id'] = result[0]['cluster_id']
    ctx.obj['cluster_rev'] = result[0]['rev']


def complete_cluster_change(ctx):
    db_query(
        ctx,
        'metadb',
        """
             SELECT code.complete_cluster_change(%(cluster_id)s, %(cluster_rev)s)
             """,
        cluster_id=ctx.obj['cluster_id'],
        cluster_rev=ctx.obj['cluster_rev'],
    )

    del ctx.obj['cluster_id']
    del ctx.obj['cluster_rev']


@contextmanager
def cluster_lock(ctx, cluster_id):
    with db_transaction(ctx, 'metadb') as txn:
        lock_cluster(ctx, cluster_id)
        yield txn
        if txn.active:
            complete_cluster_change(ctx)


def update_cluster(ctx, cluster_id, *, status=None, environment=None):
    result = db_query(
        ctx,
        'metadb',
        """
        UPDATE dbaas.clusters
        SET
        {% if status %}
            status = %(status)s,
        {% endif %}
        {% if environment %}
            env = %(environment)s,
        {% endif %}
            cid = cid
        WHERE cid = %(cluster_id)s
        RETURNING cid
        """,
        cluster_id=cluster_id,
        status=status,
        environment=environment,
        fetch_single=True,
    )

    if not result:
        raise ClusterNotFound(cluster_id)


def get_cluster_type(cluster):
    return to_cluster_type(cluster['type'])


def get_cluster_type_by_id(ctx, cluster_id):
    return get_cluster_type(get_cluster(ctx, cluster_id))


def get_cluster_flavor_by_id(ctx, cluster_id):
    hosts = get_hosts(ctx, cluster_ids=[cluster_id])
    for host in hosts:
        if 'zk' not in host['roles']:
            return host['flavor']
    raise ClickException('Non zookeeper hosts not found')
