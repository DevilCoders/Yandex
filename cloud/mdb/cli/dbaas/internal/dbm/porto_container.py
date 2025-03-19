from click import ClickException

from cloud.mdb.cli.dbaas.internal.db import db_query, db_transaction


def find_porto_container(ctx, hostname):
    containers = get_porto_containers(ctx, hostnames=[hostname])

    if len(containers) > 1:
        raise RuntimeError("Found multiple records, but it's expected to get no more than one.")

    return containers[0] if containers else None


def get_porto_container(ctx, hostname):
    container = find_porto_container(ctx, hostname)
    if not container:
        raise ClickException(f'Porto container "{hostname}" not found.')

    return container


def ensure_porto_container_exists(ctx, hostname):
    get_porto_container(ctx, hostname)


def get_porto_containers(
    ctx,
    untyped_ids=None,
    dom0_ids=None,
    hostnames=None,
    project=None,
    zones=None,
    generations=None,
    bootstrap_cmd=None,
    exclude_bootstrap_cmd=None,
    limit=None,
):
    query = """
        WITH data_volumes AS (
            SELECT
                container,
                sum(space_limit) size
            FROM mdb.volumes
            GROUP BY container
        )
        SELECT
            c.fqdn,
            c.dom0,
            c.cluster_name,
            c.project_id,
            c.managing_project_id,
            c.bootstrap_cmd,
            c.cpu_guarantee,
            c.cpu_limit,
            c.memory_guarantee,
            c.memory_limit,
            c.net_guarantee "network_guarantee",
            c.net_limit "network_limit",
            c.io_limit,
            v.size "volume_size",
            c.extra_properties
        FROM mdb.containers c
        JOIN mdb.dom0_info d ON d.fqdn = c.dom0
        JOIN data_volumes v ON v.container = c.fqdn
        WHERE true
        {% if hostnames %}
          AND c.fqdn = ANY(%(hostnames)s)
        {% endif %}
        {% if dom0_ids %}
          AND c.dom0 = ANY(%(dom0_ids)s)
        {% endif %}
        {% if project %}
          AND d.project = %(project)s
        {% endif %}
        {% if generations is not none %}
          AND d.generation = ANY(%(generations)s)
        {% endif %}
        {% if zones is not none %}
          AND d.geo = ANY(%(zones)s)
        {% endif %}
        {% if bootstrap_cmd %}
          AND c.bootstrap_cmd LIKE %(bootstrap_cmd)s
        {% endif %}
        {% if exclude_bootstrap_cmd %}
          AND c.bootstrap_cmd NOT LIKE %(exclude_bootstrap_cmd)s
        {% endif %}
        {% if untyped_ids %}
          AND (
              c.fqdn = ANY(%(untyped_ids)s)
              OR c.dom0 = ANY(%(untyped_ids)s)
        )
        {% endif %}
        {% if limit %}
        LIMIT {{ limit }}
        {% endif %}
        """
    return db_query(
        ctx,
        'dbm',
        query,
        untyped_ids=untyped_ids,
        hostnames=hostnames,
        dom0_ids=dom0_ids,
        project=project,
        zones=zones,
        generations=generations,
        bootstrap_cmd=bootstrap_cmd,
        exclude_bootstrap_cmd=exclude_bootstrap_cmd,
        limit=limit,
    )


def update_porto_container(ctx, hostname, *, bootstrap_cmd=None, extra_properties=None):
    result = db_query(
        ctx,
        'dbm',
        """
        UPDATE mdb.containers
        SET
        {% if bootstrap_cmd %}
            bootstrap_cmd = %(bootstrap_cmd)s,
        {% else %}
            bootstrap_cmd = bootstrap_cmd,
        {% endif %}
        {% if extra_properties %}
            extra_properties = %(extra_properties)s
        {% else %}
            extra_properties = extra_properties
        {% endif %}
        WHERE fqdn = %(fqdn)s
        RETURNING fqdn;
        """,
        fqdn=hostname,
        bootstrap_cmd=bootstrap_cmd,
        extra_properties=extra_properties,
    )

    if not result:
        raise ClickException(f'Porto container "{hostname}" not found.')


def delete_porto_container(ctx, fqdn):
    with db_transaction(ctx, 'dbm'):
        db_query(
            ctx,
            'dbm',
            """
                 DELETE FROM mdb.volumes
                 WHERE container = %(container_fqdn)s
                 """,
            container_fqdn=fqdn,
            fetch=False,
        )
        db_query(
            ctx,
            'dbm',
            """
                 DELETE FROM mdb.containers
                 WHERE fqdn = %(fqdn)s
                 """,
            fqdn=fqdn,
            fetch=False,
        )
