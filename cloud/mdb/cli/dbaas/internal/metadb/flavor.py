import click
from click import ClickException
from cloud.mdb.cli.dbaas.internal.db import db_query, MultipleRecordsError
from cloud.mdb.cli.dbaas.internal.metadb.common import to_db_cluster_types


class FlavorType(click.Choice):
    name = 'flavor_type'

    def __init__(self):
        super().__init__(['standard', 'burstable', 'memory-optimized', 'high-memory', 'gpu'])


def get_flavor(ctx, name=None, *, untyped_id=None):
    """
    Get flavor / resource preset from metadb.
    """
    flavors = get_flavors(ctx, name=name, untyped_id=untyped_id)

    if not flavors:
        raise ClickException(f'Resource preset "{name}" not found.')

    if len(flavors) > 1:
        raise MultipleRecordsError()

    return flavors[0]


def get_flavors(ctx, *, name=None, untyped_id=None, flavor_type=None, platform=None, generations=None, limit=None):
    """
    Get flavors / resource presets from metadb.
    """
    query = """
        SELECT
            name,
            cpu_guarantee,
            cpu_limit,
            memory_guarantee,
            memory_limit,
            network_guarantee,
            network_limit,
            io_limit,
            visible,
            vtype,
            platform_id,
            type,
            generation,
            gpu_limit,
            io_cores_limit
        FROM
            dbaas.flavors
        WHERE true
        {% if flavor_type %}
          AND type::text = %(flavor_type)s
        {% endif %}
        {% if platform %}
          AND platform_id = %(platform)s
        {% endif %}
        {% if generations %}
          AND generation = ANY(%(generations)s)
        {% endif %}
        {% if name %}
          AND name = %(name)s
        {% endif %}
        {% if untyped_id %}
          AND (name = %(untyped_id)s
               OR id::text = %(untyped_id)s)
        {% endif %}
        ORDER BY
            generation,
            type,
            cpu_limit,
            memory_limit
        {% if limit %}
        LIMIT {{ limit }}
        {% endif %}
        """
    return db_query(
        ctx,
        'metadb',
        query,
        name=name,
        untyped_id=untyped_id,
        flavor_type=flavor_type,
        platform=platform,
        generations=generations,
        limit=limit,
    )


def get_valid_resources(
    ctx, *, cluster_types=None, role=None, flavor_type=None, platform=None, generations=None, disk_type=None, limit=None
):
    """
    Get valid resources from metadb.
    """
    query = """
        SELECT
            vr.cluster_type,
            vr.role,
            f.name "flavor",
            f.cpu_guarantee,
            f.cpu_limit,
            f.memory_guarantee,
            f.memory_limit,
            array_agg(DISTINCT g.name) "zone_ids",
            array_agg(DISTINCT d.disk_type_ext_id) "disk_types",
            vr.disk_size_range,
            lower(vr.disk_size_range) "min_disk_size",
            upper(vr.disk_size_range) "max_disk_size",
            vr.disk_sizes,
            vr.min_hosts,
            vr.max_hosts,
            vr.feature_flag
        FROM
            dbaas.valid_resources vr
            JOIN dbaas.flavors f ON (f.id = vr.flavor)
            JOIN dbaas.geo g USING (geo_id)
            JOIN dbaas.disk_type d USING (disk_type_id)
        WHERE true
        {% if cluster_types %}
          AND vr.cluster_type = ANY(%(cluster_types)s::dbaas.cluster_type[])
        {% endif %}
        {% if role %}
          AND vr.role = %(role)s
        {% endif %}
        {% if flavor_type %}
          AND f.type::text = %(flavor_type)s
        {% endif %}
        {% if platform %}
          AND f.platform_id = %(platform)s
        {% endif %}
        {% if generations %}
          AND generation = ANY(%(generations)s)
        {% endif %}
        {% if disk_type %}
          AND d.disk_type_ext_id = %(disk_type)s
        {% endif %}
        GROUP BY
            vr.cluster_type,
            vr.role,
            f.name,
            f.generation,
            f.type,
            f.cpu_guarantee,
            f.cpu_limit,
            f.memory_guarantee,
            f.memory_limit,
            vr.disk_size_range,
            vr.disk_sizes,
            vr.min_hosts,
            vr.max_hosts,
            vr.feature_flag
        ORDER BY
            vr.cluster_type,
            vr.role,
            f.generation,
            f.type,
            f.cpu_guarantee,
            f.memory_guarantee
        {% if limit %}
        LIMIT {{ limit }}
        {% endif %}
        """
    return db_query(
        ctx,
        'metadb',
        query,
        cluster_types=to_db_cluster_types(cluster_types),
        role=role,
        flavor_type=flavor_type,
        platform=platform,
        generations=generations,
        disk_type=disk_type,
        limit=limit,
    )
