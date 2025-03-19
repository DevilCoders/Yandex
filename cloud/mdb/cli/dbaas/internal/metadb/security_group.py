from cloud.mdb.cli.dbaas.internal.db import db_query


def get_security_groups(ctx, *, cluster_ids=None, folder_ids=None, limit=None):
    return db_query(
        ctx,
        'metadb',
        """
                    SELECT
                        sg_ext_id "id",
                        cid "cluster_id",
                        sg_type "type",
                        sg_hash "hash",
                        sg_allow_all "allow_all"
                    FROM dbaas.sgroups sg
                    JOIN dbaas.clusters c USING (cid)
                    JOIN dbaas.folders f USING (folder_id)
                    WHERE true
                    {% if folder_ids %}
                      AND f.folder_ext_id = ANY(%(folder_ids)s)
                    {% endif %}
                    {% if cluster_ids %}
                      AND sg.cid = ANY(%(cluster_ids)s)
                    {% endif %}
                    {% if limit %}
                    LIMIT {{ limit }}
                    {% endif %}
                    """,
        folder_ids=folder_ids,
        cluster_ids=cluster_ids,
        limit=limit,
    )
