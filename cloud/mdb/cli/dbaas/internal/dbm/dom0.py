from click import ClickException

from cloud.mdb.cli.dbaas.internal.db import db_query, MultipleRecordsError


def get_dom0_server(ctx, untyped_id):
    """
    Get dom0 server from metadb.
    """
    dom0_servers = get_dom0_servers(ctx, untyped_ids=[untyped_id])

    if not dom0_servers:
        raise ClickException('Dom0 server not found.')

    if len(dom0_servers) > 1:
        raise MultipleRecordsError()

    return dom0_servers[0]


def get_dom0_servers(
    ctx,
    untyped_ids=None,
    dom0_ids=None,
    hostnames=None,
    project=None,
    zones=None,
    generations=None,
    free_cpu=None,
    free_memory=None,
    free_ssd=None,
    free_hdd=None,
    free_raw_disks=None,
    allow_new_hosts=None,
    limit=None,
):
    """
    Get dom0 servers from metadb.
    """
    query = """
        SELECT
            fqdn "id",
            project,
            generation,
            geo,
            free_cores "cpu_free",
            total_cores "cpu_total",
            free_memory "memory_free",
            total_memory "memory_total",
            free_net "network_free",
            total_net "network_total",
            free_io "io_free",
            total_io "io_total",
            free_sata "hdd_space_free",
            total_sata "hdd_space_total",
            free_ssd "ssd_space_free",
            total_ssd "ssd_space_total",
            free_raw_disks "raw_disks_free",
            total_raw_disks "raw_disks_total",
            free_raw_disks_space "raw_disks_space_free",
            total_raw_disks_space "raw_disks_space_total",
            allow_new_hosts
        FROM mdb.dom0_info d
        WHERE true
        {% if dom0_ids %}
          AND fqdn = ANY(%(dom0_ids)s)
        {% endif %}
        {% if project %}
          AND project = %(project)s
        {% endif %}
        {% if generations is not none %}
              AND generation = ANY(%(generations)s)
        {% endif %}
        {% if zones is not none %}
              AND geo = ANY(%(zones)s)
        {% endif %}
        {% if hostnames %}
          AND EXISTS (SELECT 1
                      FROM mdb.containers
                      WHERE dom0 = d.fqdn AND fqdn = ANY(%(hostnames)s))
        {% endif %}
        {% if untyped_ids %}
          AND (
              fqdn = ANY(%(untyped_ids)s)
              OR EXISTS (SELECT 1
                         FROM mdb.containers
                         WHERE dom0 = d.fqdn AND fqdn = ANY(%(untyped_ids)s))
        )
        {% endif %}
        {% if free_cpu %}
          AND free_cores >= %(free_cpu)s
        {% endif %}
        {% if free_memory %}
          AND free_memory >= %(free_memory)s
        {% endif %}
        {% if free_ssd %}
          AND free_ssd >= %(free_ssd)s
        {% endif %}
        {% if free_hdd %}
          AND free_sata >= %(free_hdd)s
        {% endif %}
        {% if free_raw_disks %}
          AND free_raw_disks >= %(free_raw_disks)s
        {% endif %}
        {% if allow_new_hosts is not none %}
              AND allow_new_hosts = %(allow_new_hosts)s
        {% endif %}
        ORDER BY generation, fqdn
        {% if limit %}
        LIMIT {{ limit }}
        {% endif %}
        """
    result = db_query(
        ctx,
        'dbm',
        query,
        untyped_ids=untyped_ids,
        dom0_ids=dom0_ids,
        hostnames=hostnames,
        project=project,
        zones=zones,
        generations=generations,
        free_cpu=free_cpu,
        free_memory=free_memory,
        free_ssd=free_ssd,
        free_hdd=free_hdd,
        free_raw_disks=free_raw_disks,
        allow_new_hosts=allow_new_hosts,
        limit=limit,
    )

    def sort_func(item: dict) -> tuple:
        groups = {'memory_free': free_memory, 'cpu_free': free_cpu, 'ssd_space_free': free_ssd}
        for group_name in groups:
            if groups[group_name] is None:
                continue
            groups[group_name] = item[group_name]
        return groups['memory_free'], groups['cpu_free'], groups['ssd_space_free']

    result.sort(key=sort_func, reverse=True)
    return result


def get_dom0_server_stats(ctx, *, project=None, generations=None, zones=None, allow_new_hosts=None):
    query = """
        SELECT
            generation,
            geo,
            count(1) dom0_servers,
            sum(free_cores) "cpu_free",
            sum(total_cores) "cpu_total",
            sum(free_memory) "memory_free",
            sum(total_memory) "memory_total",
            sum(free_net) "network_free",
            sum(total_net) "network_total",
            sum(free_io) "io_free",
            sum(total_io) "io_total",
            sum(free_sata) "hdd_space_free",
            sum(total_sata) "hdd_space_total",
            sum(free_ssd) "ssd_space_free",
            sum(total_ssd) "ssd_space_total",
            sum(free_raw_disks) "raw_disks_free",
            sum(total_raw_disks) "raw_disks_total",
            sum(free_raw_disks_space) "raw_disks_space_free",
            sum(total_raw_disks_space) "raw_disks_space_total"
        FROM mdb.dom0_info
        WHERE true
    {% if project %}
          AND project = %(project)s
    {% endif %}
    {% if generations is not none %}
          AND generation = ANY(%(generations)s)
    {% endif %}
    {% if zones is not none %}
          AND geo = ANY(%(zones)s)
    {% endif %}
    {% if allow_new_hosts is not none %}
          AND allow_new_hosts = %(allow_new_hosts)s
    {% endif %}
        GROUP BY ROLLUP (generation, geo)
        ORDER BY generation, geo
        """
    return db_query(
        ctx, 'dbm', query, project=project, generations=generations, zones=zones, allow_new_hosts=allow_new_hosts
    )
