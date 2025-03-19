from cloud.mdb.cli.dbaas.internal.db import db_query
from cloud.mdb.cli.dbaas.internal.metadb.cluster import cluster_lock


def get_component_version(
    ctx,
    cluster_id,
    component,
):
    return db_query(
        ctx,
        'metadb',
        """
        SELECT minor_version, package_version FROM dbaas.versions WHERE cid = %(cluster_id)s AND component = %(component)s
        """,
        cluster_id=cluster_id,
        component=component,
    )


def update_component_version(
    ctx,
    cluster_id,
    component,
    minor_version,
    package_version,
):
    with cluster_lock(ctx, cluster_id):
        db_query(
            ctx,
            'metadb',
            """
            UPDATE dbaas.versions SET package_version = %(package_version)s, minor_version = %(minor_version)s WHERE cid = %(cluster_id)s AND component = %(component)s
            """,
            cluster_id=cluster_id,
            component=component,
            minor_version=minor_version,
            package_version=package_version,
            fetch=False,
        )
        return get_component_version(ctx, cluster_id, component)
