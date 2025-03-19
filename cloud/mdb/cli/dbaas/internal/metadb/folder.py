from click import ClickException

from cloud.mdb.cli.dbaas.internal.db import db_query, MultipleRecordsError


def get_folder(ctx, folder_id=None, *, untyped_id=None):
    assert sum(bool(v) for v in (folder_id, untyped_id)) == 1

    folders = get_folders(ctx, folder_ids=[folder_id] if folder_id else None, untyped_id=untyped_id)

    if not folders:
        if folder_id:
            raise ClickException(f'Folder {folder_id} not found.')
        else:
            raise ClickException('Folder not found.')

    if len(folders) > 1:
        raise MultipleRecordsError()

    return folders[0]


def get_folders(ctx, *, untyped_id=None, cloud_id=None, folder_ids=None, cluster_ids=None, limit=None):
    return db_query(
        ctx,
        'metadb',
        """
                    SELECT
                        f.folder_ext_id "id",
                        cl.cloud_ext_id "cloud_id"
                    FROM dbaas.folders f
                    JOIN dbaas.clouds cl USING (cloud_id)
                    WHERE true
                    {% if cloud_id %}
                      AND cl.cloud_ext_id = %(cloud_id)s
                    {% endif %}
                    {% if folder_ids %}
                      AND f.folder_ext_id = ANY(%(folder_ids)s)
                    {% endif %}
                    {% if cluster_ids %}
                      AND EXISTS (SELECT 1
                                  FROM dbaas.clusters
                                  WHERE folder_id = f.folder_id AND cid = ANY(%(cluster_ids)s))
                    {% endif %}
                    {% if untyped_id %}
                      AND f.folder_ext_id = %(untyped_id)s
                       OR EXISTS (SELECT 1
                                  FROM dbaas.clusters
                                  WHERE folder_id = f.folder_id AND cid = %(untyped_id)s)
                    {% endif %}
                    {% if limit %}
                    LIMIT {{ limit }}
                    {% endif %}
                    """,
        untyped_id=untyped_id,
        cloud_id=cloud_id,
        folder_ids=folder_ids,
        cluster_ids=cluster_ids,
        limit=limit,
    )
