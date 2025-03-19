from cloud.mdb.cli.dbaas.internal.db import db_query


def get_transfers(ctx, limit=None):
    query = """
        SELECT
            t.id,
            t.src_dom0,
            t.src_dom0,
            t.dest_dom0,
            t.container,
            t.placeholder,
            t.started
        FROM mdb.transfers t
        {% if limit %}
        LIMIT {{ limit }}
        {% endif %}
        """
    return db_query(ctx, 'dbm', query, limit=limit)
