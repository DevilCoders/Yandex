from cloud.mdb.cli.dbaas.internal.db import db_query
from cloud.mdb.cli.dbaas.internal.utils import generate_id


def create_backup_task(ctx, *, cluster_id, shard_id=None):
    backup_id = generate_id(ctx)

    db_query(
        ctx,
        'metadb',
        """
        SELECT * FROM code.plan_managed_backup(
            i_backup_id      => %(backup_id)s,
            i_cid            => %(cluster_id)s,
            i_subcid         => (SELECT subcid from dbaas.subclusters where cid = %(cluster_id)s),
            i_shard_id       => %(shard_id)s,
            i_status         => 'PLANNED'::dbaas.backup_status,
            i_method         => 'FULL'::dbaas.backup_method,
            i_initiator      => 'SCHEDULE'::dbaas.backup_initiator,
            i_delayed_until  => NOW(),
            i_scheduled_date => DATE(NOW()),
            i_parent_ids     => NULL,
            i_child_id       => NULL
        )
        """,
        cluster_id=cluster_id,
        shard_id=shard_id,
        backup_id=backup_id,
    )

    return backup_id
