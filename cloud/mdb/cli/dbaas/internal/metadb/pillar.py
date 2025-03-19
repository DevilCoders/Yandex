import json

from click import ClickException
from cloud.mdb.cli.dbaas.internal.common import delete_key, expand_key_pattern, get_key, update_key
from cloud.mdb.cli.dbaas.internal.db import db_query, db_transaction
from cloud.mdb.cli.dbaas.internal.metadb.common import to_db_cluster_type
from cloud.mdb.cli.dbaas.internal.metadb.exceptions import PillarNotFound
from cloud.mdb.cli.dbaas.internal.metadb.host import to_overlay_fqdn

PILLAR_KEY_SEPARATORS = [':', ',']


def get_pillar(
    ctx,
    *,
    untyped_id=None,
    cluster_type=None,
    cluster_id=None,
    subcluster_id=None,
    shard_id=None,
    host=None,
    target_id=None,
    revision=None,
    jsonpath_expression=None,
):
    """
    Get pillar from metadb.
    """
    assert sum(bool(v) for v in (untyped_id, cluster_type, cluster_id, subcluster_id, shard_id, host)) == 1

    if host:
        host = to_overlay_fqdn(ctx, host)

    query = """
        SELECT
        {% if jsonpath_expression %}
            jsonb_path_query_array(value, %(jsonpath_expression)s) "value"
        {% else %}
            value
        {% endif %}
        {% if cluster_type %}
        FROM dbaas.cluster_type_pillar
        WHERE type = %(cluster_type)s::dbaas.cluster_type
        {% elif target_id %}
        FROM dbaas.target_pillar
        {% else %}
        {%   if revision %}
        FROM dbaas.pillar_revs
        {%   else %}
        FROM dbaas.pillar
        {%   endif %}
        {% endif %}
        {% if cluster_id %}
        WHERE cid = %(cluster_id)s
        {% elif subcluster_id %}
        WHERE subcid = %(subcluster_id)s
        {% elif shard_id %}
        WHERE shard_id = %(shard_id)s
        {% elif host %}
        WHERE fqdn = %(host)s
        {% elif untyped_id %}
        WHERE (
            cid::text = %(untyped_id)s
            OR subcid::text = %(untyped_id)s
            OR shard_id::text = %(shard_id)s
            OR fqdn = %(untyped_id)s
        )
        {% endif %}
        {% if target_id %}
          AND target_id = %(target_id)s
        {% endif %}
        {% if revision %}
          AND rev = %(revision)s
        {% endif %}
        """
    result = db_query(
        ctx,
        'metadb',
        query,
        untyped_id=untyped_id,
        cluster_type=to_db_cluster_type(cluster_type),
        cluster_id=cluster_id,
        subcluster_id=subcluster_id,
        shard_id=shard_id,
        host=host,
        target_id=target_id,
        revision=revision,
        jsonpath_expression=jsonpath_expression,
    )

    if not result:
        raise PillarNotFound(
            cluster_type=cluster_type, cluster_id=cluster_id, subcluster_id=subcluster_id, shard_id=shard_id, host=host
        )

    return result[0]['value']


def get_pillar_history(
    ctx,
    *,
    from_revision=None,
    to_revision=None,
    cluster_id=None,
    subcluster_id=None,
    shard_id=None,
    host=None,
):
    """
    Get pillar history from metadb.
    """
    assert sum(bool(v) for v in (cluster_id, subcluster_id, shard_id, host)) == 1

    if host:
        host = to_overlay_fqdn(ctx, host)

    query = """
        SELECT
            cc.rev "revision",
            cc.committed_at "timestamp",
            pr.value "pillar"
        {% if cluster_id %}
        FROM dbaas.clusters_changes cc
        JOIN dbaas.pillar_revs pr ON (pr.rev = cc.rev AND pr.cid = cc.cid)
        WHERE cc.cid = %(cluster_id)s
        {% elif subcluster_id %}
        FROM dbaas.subclusters sc
        JOIN dbaas.clusters_changes cc USING (cid)
        JOIN dbaas.pillar_revs pr ON (pr.rev = cc.rev AND pr.subcid = sc.subcid)
        WHERE sc.subcid = %(subcluster_id)s
        {% elif shard_id %}
        FROM dbaas.shards s
        FROM dbaas.subclusters sc USING (subcid)
        JOIN dbaas.clusters_changes cc USING (cid)
        JOIN dbaas.pillar_revs pr ON (pr.rev = cc.rev AND pr.shard_id = s.shard_id)
        WHERE s.shard_id = %(shard_id)s
        {% elif host %}
        FROM dbaas.hosts h
        JOIN dbaas.subclusters sc USING (subcid)
        JOIN dbaas.clusters_changes cc USING (cid)
        JOIN dbaas.pillar_revs pr ON (pr.rev = cc.rev AND pr.fqdn = h.fqdn)
        WHERE h.fqdn = %(host)s
        {% endif %}
        {% if from_revision %}
          AND cc.rev >= %(from_revision)s
        {% endif %}
        {% if to_revision %}
          AND cc.rev <= %(to_revision)s
        {% endif %}
        ORDER BY revision
        """
    result = db_query(
        ctx,
        'metadb',
        query,
        cluster_id=cluster_id,
        subcluster_id=subcluster_id,
        shard_id=shard_id,
        host=host,
        from_revision=from_revision,
        to_revision=to_revision,
    )

    if not result:
        raise PillarNotFound(cluster_id=cluster_id, subcluster_id=subcluster_id, shard_id=shard_id, host=host)

    return result


def create_pillar(ctx, value, *, cluster_id=None, subcluster_id=None, shard_id=None, host=None):
    """
    Create pillar in metadb.
    """
    assert sum(bool(v) for v in (cluster_id, subcluster_id, shard_id, host)) == 1

    if host:
        host = to_overlay_fqdn(ctx, host)

    db_query(
        ctx,
        'metadb',
        """
             SELECT code.add_pillar(
                 i_cid   => %(cluster_id)s,
                 i_rev   => %(cluster_rev)s,
                 i_key   => code.make_pillar_key(
                     i_cid      => %(pillar_cluster_id)s,
                     i_subcid   => %(pillar_subcluster_id)s,
                     i_shard_id => %(pillar_shard_id)s,
                     i_fqdn     => %(pillar_host)s),
                 i_value => %(value)s);
             """,
        cluster_id=ctx.obj['cluster_id'],
        cluster_rev=ctx.obj['cluster_rev'],
        pillar_cluster_id=cluster_id,
        pillar_subcluster_id=subcluster_id,
        pillar_shard_id=shard_id,
        pillar_host=host,
        value=json.dumps(value),
    )


def update_pillar(ctx, value, cluster_id=None, subcluster_id=None, shard_id=None, host=None):
    """
    Update pillar in metadb.
    """
    assert sum(bool(v) for v in (cluster_id, subcluster_id, shard_id, host)) == 1

    if host:
        host = to_overlay_fqdn(ctx, host)

    db_query(
        ctx,
        'metadb',
        """
             SELECT code.update_pillar(
                 i_cid   => %(cluster_id)s,
                 i_rev   => %(cluster_rev)s,
                 i_key   => code.make_pillar_key(
                     i_cid      => %(pillar_cluster_id)s,
                     i_subcid   => %(pillar_subcluster_id)s,
                     i_shard_id => %(pillar_shard_id)s,
                     i_fqdn     => %(pillar_host)s),
                 i_value => %(value)s);
             """,
        cluster_id=ctx.obj['cluster_id'],
        cluster_rev=ctx.obj['cluster_rev'],
        pillar_cluster_id=cluster_id,
        pillar_subcluster_id=subcluster_id,
        pillar_shard_id=shard_id,
        pillar_host=host,
        value=json.dumps(value),
    )


def delete_pillar(ctx, *, cluster_id=None, subcluster_id=None, shard_id=None, host=None):
    """
    Delete pillar from metadb.
    """
    assert sum(bool(v) for v in (cluster_id, subcluster_id, shard_id, host)) == 1

    host = to_overlay_fqdn(ctx, host)

    query = """
        DELETE FROM dbaas.pillar
        {% if cluster_id %}
        WHERE cid = %(cluster_id)s
        {% elif subcluster_id %}
        WHERE subcid = %(subcluster_id)s
        {% elif shard_id %}
        WHERE shard_id = %(shard_id)s
        {% elif host %}
        WHERE fqdn = %(host)s
        {% endif %}
        """
    db_query(
        ctx,
        'metadb',
        query,
        cluster_id=cluster_id,
        subcluster_id=subcluster_id,
        shard_id=shard_id,
        host=host,
        fetch=False,
    )


def get_pillar_keys(
    ctx,
    key,
    *,
    untyped_id=None,
    cluster_type=None,
    cluster_id=None,
    subcluster_id=None,
    shard_id=None,
    host=None,
    revision=None,
):
    pillar = get_pillar(
        ctx,
        untyped_id=untyped_id,
        cluster_type=cluster_type,
        cluster_id=cluster_id,
        subcluster_id=subcluster_id,
        shard_id=shard_id,
        host=host,
        revision=revision,
    )

    return [
        get_key(pillar, key_path, separators=PILLAR_KEY_SEPARATORS)
        for key_path in expand_key_pattern(pillar, key, separators=PILLAR_KEY_SEPARATORS)
    ]


def update_pillar_key(ctx, key, value, cluster_id=None, subcluster_id=None, shard_id=None, host=None, force=None):
    with db_transaction(ctx, 'metadb'):
        pillar = get_pillar(ctx, cluster_id=cluster_id, subcluster_id=subcluster_id, shard_id=shard_id, host=host)

        for item in expand_key_pattern(pillar, key, separators=PILLAR_KEY_SEPARATORS):
            if not force:
                current_value = get_key(pillar, item, default=None, separators=PILLAR_KEY_SEPARATORS)
                if current_value is not None and value is not None and type(current_value) != type(value):
                    raise ClickException(
                        f'Cannot update pillar item {item} of type {type(current_value).__name__} to the value'
                        f' of type {type(value).__name__}. Please use force mode in order to update pillar item'
                        ' with type override.'
                    )

            update_key(pillar, item, value, separators=PILLAR_KEY_SEPARATORS)

        update_pillar(
            ctx,
            pillar,
            cluster_id=cluster_id,
            subcluster_id=subcluster_id,
            shard_id=shard_id,
            host=host,
        )


def delete_pillar_key(ctx, key, cluster_id=None, subcluster_id=None, shard_id=None, host=None):
    with db_transaction(ctx, 'metadb'):
        pillar = get_pillar(ctx, cluster_id=cluster_id, subcluster_id=subcluster_id, shard_id=shard_id, host=host)

        for item in expand_key_pattern(pillar, key, separators=PILLAR_KEY_SEPARATORS):
            delete_key(pillar, item, separators=PILLAR_KEY_SEPARATORS)

        update_pillar(
            ctx,
            pillar,
            cluster_id=cluster_id,
            subcluster_id=subcluster_id,
            shard_id=shard_id,
            host=host,
        )
