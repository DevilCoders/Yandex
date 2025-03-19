from cloud.mdb.cli.dbaas.internal.db import db_query


def get_volumes(ctx, hostnames=None, dom0_ids=None, limit=None):
    query = """
        SELECT
            container,
            path,
            dom0,
            dom0_path,
            read_only,
            space_limit "size"
        FROM mdb.volumes
        WHERE true
        {% if containers %}
          AND container = ANY(%(containers)s)
        {% endif %}
        {% if dom0_ids %}
          AND dom0 = ANY(%(dom0_ids)s)
        {% endif %}
        {% if limit %}
        LIMIT {{ limit }}
        {% endif %}
        """
    return db_query(ctx, 'dbm', query, containers=hostnames, dom0_ids=dom0_ids, limit=limit)
