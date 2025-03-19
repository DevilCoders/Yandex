from click import ClickException

from cloud.mdb.cli.dbaas.internal.db import db_query


def get_zone(ctx, name):
    query = """
        SELECT
            g.geo_id "id",
            g.name,
            r.name "region"
        FROM dbaas.geo g
        JOIN dbaas.regions r USING (region_id)
        WHERE g.name = %(name)s
        """
    result = db_query(ctx, 'metadb', query, name=name)

    if not result:
        raise ClickException(f'Availability zone "{name}" not found.')

    return result[0]


def get_zones(ctx):
    query = """
        SELECT
            g.geo_id "id",
            g.name,
            r.name "region"
        FROM dbaas.geo g
        JOIN dbaas.regions r USING (region_id)
        """
    return db_query(ctx, 'metadb', query)
