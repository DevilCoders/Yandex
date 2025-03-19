from click import ClickException

from cloud.mdb.cli.dbaas.internal.db import db_query


def get_cloud(ctx, cloud_id):
    query = """
        SELECT
            cloud_ext_id "id",
            clusters_used,
            clusters_quota,
            cpu_used,
            cpu_quota,
            gpu_used,
            gpu_quota,
            memory_used,
            memory_quota,
            hdd_space_used,
            hdd_space_quota,
            ssd_space_used,
            ssd_space_quota
        FROM dbaas.clouds cl
        WHERE cloud_ext_id = %(cloud_id)s
        """
    result = db_query(ctx, 'metadb', query, cloud_id=cloud_id)

    if not result:
        raise ClickException(f'Cloud "{cloud_id}" not found.')

    return result[0]


def get_clouds(ctx, cluster_ids=None, limit=None):
    query = """
        SELECT
            cloud_ext_id "id",
            clusters_used,
            clusters_quota,
            cpu_used,
            cpu_quota,
            gpu_used,
            gpu_quota,
            memory_used,
            memory_quota,
            hdd_space_used,
            hdd_space_quota,
            ssd_space_used,
            ssd_space_quota
        FROM dbaas.clouds cl
        {% if cluster_ids %}
        WHERE EXISTS (SELECT 1
                      FROM dbaas.clusters
                      JOIN dbaas.folders USING (folder_id)
                      WHERE cloud_id = cl.cloud_id AND cid = ANY(%(cluster_ids)s))
        {% endif %}
        {% if limit %}
        LIMIT {{ limit }}
        {% endif %}
        """
    return db_query(ctx, 'metadb', query, cluster_ids=cluster_ids, limit=limit)


def get_cloud_features(ctx, cloud_id):
    query = """
        SELECT
            array_agg(flag_name) "features"
        FROM dbaas.clouds
        JOIN dbaas.cloud_feature_flags USING (cloud_id)
        WHERE cloud_ext_id = %(cloud_id)s
        """
    result = db_query(ctx, 'metadb', query, cloud_id=cloud_id)

    if result:
        return result[0]['features'] or []
    else:
        return []


def enable_feature_on_cloud(ctx, cloud_id, feature):
    query = """
        INSERT INTO dbaas.cloud_feature_flags (cloud_id, flag_name)
        SELECT
            cl.cloud_id,
            %(feature)s
        FROM dbaas.clouds cl
        WHERE cl.cloud_ext_id = %(cloud_id)s
          AND NOT EXISTS (SELECT 1 FROM dbaas.cloud_feature_flags
                          WHERE cloud_id = cl.cloud_id AND flag_name = %(feature)s )
        """
    db_query(ctx, 'metadb', query, cloud_id=cloud_id, feature=feature, fetch=False)


def disable_feature_on_cloud(ctx, cloud_id, feature):
    query = """
        DELETE FROM dbaas.cloud_feature_flags
        WHERE flag_name = %(feature)s
          AND cloud_id IN (SELECT cloud_id FROM dbaas.clouds
                           WHERE cloud_ext_id = %(cloud_id)s)
        """
    db_query(ctx, 'metadb', query, cloud_id=cloud_id, feature=feature, fetch=False)


def update_cloud_quota(
    ctx,
    cloud_id,
    add_cpu=None,
    add_gpu=None,
    add_memory=None,
    add_disk_space=None,
    disk_quota_type=None,
    add_ssd_space=None,
    add_hdd_space=None,
    add_clusters=None,
):
    """
    Updates cloud quota in metadb.
    """
    add_disk_quotas = DiskQuotas(add_disk_space, disk_quota_type, add_ssd_space, add_hdd_space)

    query = """
        SELECT code.update_cloud_quota(
            i_cloud_ext_id => %(cloud_id)s,
            i_delta        => code.make_quota(
                i_cpu       => coalesce(%(add_cpu)s, 0.0)::real,
                i_gpu       => coalesce(%(add_gpu)s, 0)::bigint,
                i_memory    => coalesce(%(add_memory)s, 0)::bigint,
                i_ssd_space => coalesce(%(add_ssd_space)s, 0)::bigint,
                i_hdd_space => coalesce(%(add_hdd_space)s, 0)::bigint,
                i_clusters  => coalesce(%(add_clusters)s, 0)::bigint
            ),
            i_x_request_id => NULL
        )
        """

    result = db_query(
        ctx,
        'metadb',
        query,
        cloud_id=cloud_id,
        add_cpu=add_cpu,
        add_gpu=add_gpu,
        add_memory=add_memory,
        add_ssd_space=add_disk_quotas.ssd_space,
        add_hdd_space=add_disk_quotas.hdd_space,
        add_clusters=add_clusters,
        fetch_single=True,
    )

    if not result:
        raise ClickException(f'Cloud "{cloud_id}" not found.')


def update_cloud_used_resources(
    ctx,
    cloud_id,
    add_cpu=None,
    add_gpu=None,
    add_memory=None,
    add_disk_space=None,
    disk_quota_type=None,
    add_ssd_space=None,
    add_hdd_space=None,
    add_clusters=None,
):
    """
    Updates cloud's used resources in metadb.
    """
    add_disk_quotas = DiskQuotas(add_disk_space, disk_quota_type, add_ssd_space, add_hdd_space)

    query = """
        SELECT code.update_cloud_usage(
            i_cloud_id   => cloud_id,
            i_delta      => code.make_quota(
                i_cpu       => coalesce(%(add_cpu)s, 0.0)::real,
                i_gpu       => coalesce(%(add_gpu)s, 0)::bigint,
                i_memory    => coalesce(%(add_memory)s, 0)::bigint,
                i_ssd_space => coalesce(%(add_ssd_space)s, 0)::bigint,
                i_hdd_space => coalesce(%(add_hdd_space)s, 0)::bigint,
                i_clusters  => coalesce(%(add_clusters)s, 0)::bigint
            ),
            i_x_request_id => NULL
        )
        FROM dbaas.clouds
        WHERE cloud_ext_id = %(cloud_id)s
        """

    result = db_query(
        ctx,
        'metadb',
        query,
        cloud_id=cloud_id,
        add_cpu=add_cpu,
        add_gpu=add_gpu,
        add_memory=add_memory,
        add_ssd_space=add_disk_quotas.ssd_space,
        add_hdd_space=add_disk_quotas.hdd_space,
        add_clusters=add_clusters,
        fetch_single=True,
    )

    if not result:
        raise ClickException(f'Cloud "{cloud_id}" not found.')


class DiskQuotas:
    def __init__(self, disk_space, disk_type, ssd_space, hdd_space):
        if ssd_space is None:
            ssd_space = 0
        if hdd_space is None:
            hdd_space = 0
        if disk_space:
            if disk_type == 'ssd':
                ssd_space += disk_space
            elif disk_type == 'hdd':
                hdd_space += disk_space
            else:
                raise RuntimeError(f'Unexpected disk quota type: {disk_type}')

        self.ssd_space = ssd_space
        self.hdd_space = hdd_space
