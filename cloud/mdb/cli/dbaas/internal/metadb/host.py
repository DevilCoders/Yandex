from collections import OrderedDict

from cloud.mdb.cli.dbaas.internal.common import to_overlay_fqdn, to_underlay_fqdn
from cloud.mdb.cli.dbaas.internal.db import db_query, MultipleRecordsError
from cloud.mdb.cli.dbaas.internal.metadb.common import to_cluster_type, to_db_cluster_types
from cloud.mdb.cli.dbaas.internal.metadb.exceptions import HostNotFound
from cloud.mdb.cli.dbaas.internal.metadb.task import create_task
from cloud.mdb.cli.dbaas.internal.utils import STOPPED_CLUSTER_STATUS


def get_host(ctx, hostname=None, *, vtype_id=None, untyped_id=None):
    """
    Get host from metadb.
    """
    assert sum(bool(v) for v in (hostname, vtype_id, untyped_id)) == 1

    hosts = get_hosts(
        ctx,
        hostnames=[hostname] if hostname else None,
        vtype_ids=[vtype_id] if vtype_id else None,
        untyped_ids=[untyped_id] if untyped_id else None,
    )

    if not hosts:
        raise HostNotFound(hostname)

    if len(hosts) > 1:
        raise MultipleRecordsError()

    return hosts[0]


def reload_host(ctx, host):
    return get_host(ctx, host['public_fqdn'])


def get_hosts(
    ctx,
    untyped_ids=None,
    folder_id=None,
    cluster_id=None,
    cluster_ids=None,
    exclude_cluster_ids=None,
    subcluster_id=None,
    subcluster_ids=None,
    shard_id=None,
    shard_ids=None,
    hostnames=None,
    vtype=None,
    vtype_ids=None,
    exclude_vtype_ids=None,
    task_ids=None,
    role=None,
    flavor=None,
    generations=None,
    disk_type=None,
    zones=None,
    cluster_env=None,
    cluster_types=None,
    cluster_statuses=None,
    exclude_cluster_statuses=None,
    limit=None,
):
    """
    Get hosts from metadb.
    """
    if hostnames:
        hostnames = [to_overlay_fqdn(ctx, fqdn) for fqdn in hostnames]

    if untyped_ids:
        untyped_ids = [to_overlay_fqdn(ctx, fqdn) for fqdn in untyped_ids]

    query = """
        SELECT
            h.fqdn "public_fqdn",
            h.vtype_id,
            fl.vtype,
            sc.roles,
            g.name "zone",
            fl.name "flavor",
            fl.cpu_guarantee "cpu_cores",
            fl.gpu_limit "gpu_cores",
            fl.memory_guarantee "memory_size",
            d.disk_type_ext_id "disk_type",
            d.quota_type "disk_quota_type",
            h.space_limit "disk_size",
            h.created_at,
            sc.cid "cluster_id",
            c.name "cluster_name",
            c.type "cluster_type",
            c.status "cluster_status",
            sc.subcid "subcluster_id",
            h.shard_id,
            s.name "shard_name",
            h.subnet_id,
            h.assign_public_ip,
            c.env
        FROM
            dbaas.hosts h
            JOIN dbaas.subclusters sc USING (subcid)
            JOIN dbaas.clusters c USING (cid)
            JOIN dbaas.folders f USING (folder_id)
            LEFT JOIN dbaas.shards s USING (shard_id)
            JOIN dbaas.geo g USING (geo_id)
            JOIN dbaas.disk_type d USING (disk_type_id)
            JOIN dbaas.flavors fl ON fl.id = h.flavor
        WHERE true
        {% if folder_id %}
          AND f.folder_ext_id = %(folder_id)s
        {% endif %}
        {% if cluster_id %}
          AND sc.cid = %(cluster_id)s
        {% endif %}
        {% if cluster_ids %}
          AND sc.cid = ANY(%(cluster_ids)s)
        {% endif %}
        {% if exclude_cluster_ids %}
          AND sc.cid != ALL(%(exclude_cluster_ids)s)
        {% endif %}
        {% if subcluster_id %}
          AND sc.subcid = %(subcluster_id)s
        {% endif %}
        {% if subcluster_ids %}
          AND sc.subcid = ANY(%(subcluster_ids)s)
        {% endif %}
        {% if shard_id %}
          AND h.shard_id = %(shard_id)s
        {% endif %}
        {% if shard_ids %}
          AND h.shard_id = ANY(%(shard_ids)s)
        {% endif %}
        {% if hostnames %}
          AND h.fqdn = ANY(%(hostnames)s)
        {% endif %}
        {% if vtype %}
          AND fl.vtype::text = %(vtype)s
        {% endif %}
        {% if vtype_ids %}
          AND h.vtype_id = ANY(%(vtype_ids)s)
        {% endif %}
        {% if exclude_vtype_ids %}
          AND h.vtype_id != ALL(%(exclude_vtype_ids)s)
        {% endif %}
        {% if cluster_env %}
          AND c.env::text = %(cluster_env)s
        {% endif %}
        {% if cluster_types %}
          AND c.type = ANY(%(cluster_types)s::dbaas.cluster_type[])
        {% endif %}
        {% if cluster_statuses %}
          AND c.status = ANY(%(cluster_statuses)s::dbaas.cluster_status[])
        {% endif %}
        {% if exclude_cluster_statuses %}
          AND c.status != ALL(%(exclude_cluster_statuses)s::dbaas.cluster_status[])
        {% endif %}
        {% if role %}
          AND %(role)s = ANY(sc.roles::text[])
        {% endif %}
        {% if flavor %}
          AND fl.name = %(flavor)s
        {% endif %}
        {% if generations %}
          AND fl.generation = ANY(%(generations)s)
        {% endif %}
        {% if disk_type %}
          AND d.disk_type_ext_id::text = %(disk_type)s
        {% endif %}
        {% if zones %}
          AND g.name = ANY(%(zones)s)
        {% endif %}
        {% if task_ids %}
          AND EXISTS (SELECT 1
                      FROM dbaas.worker_queue
                      WHERE cid = c.cid AND task_id = ANY(%(task_ids)s))
        {% endif %}
        {% if untyped_ids %}
          AND (
              c.cid = ANY(%(untyped_ids)s)
              OR sc.subcid = ANY(%(untyped_ids)s)
              OR h.shard_id = ANY(%(untyped_ids)s)
              OR h.fqdn = ANY(%(untyped_ids)s)
              OR h.vtype_id = ANY(%(untyped_ids)s)
        )
        {% endif %}
       ORDER BY c.created_at DESC
        {% if limit %}
        LIMIT {{ limit }}
        {% endif %}
        """
    hosts = db_query(
        ctx,
        'metadb',
        query,
        untyped_ids=untyped_ids,
        folder_id=folder_id,
        cluster_id=cluster_id,
        cluster_ids=cluster_ids,
        exclude_cluster_ids=exclude_cluster_ids,
        subcluster_id=subcluster_id,
        subcluster_ids=subcluster_ids,
        shard_id=shard_id,
        shard_ids=shard_ids,
        hostnames=hostnames,
        vtype=vtype,
        vtype_ids=vtype_ids,
        exclude_vtype_ids=exclude_vtype_ids,
        task_ids=task_ids,
        cluster_env=cluster_env,
        cluster_types=to_db_cluster_types(cluster_types),
        cluster_statuses=cluster_statuses,
        exclude_cluster_statuses=exclude_cluster_statuses,
        role=role,
        generations=generations,
        flavor=flavor,
        disk_type=disk_type,
        zones=zones,
        limit=limit,
    )

    result = []
    for host in hosts:
        result.append(OrderedDict(fqdn=to_underlay_fqdn(ctx, host['public_fqdn']), **host))

    return result


def filter_hosts(hosts, subcluster_id=None, shard_id=None, role=None):
    result = []
    for host in hosts:
        if subcluster_id and host['subcluster_id'] != subcluster_id:
            continue
        if shard_id and host['shard_id'] != shard_id:
            continue
        if role and role not in host['roles']:
            continue

        result.append(host)

    return result


def get_host_stats(ctx, *, cluster_types, role=None, zones=None):
    query = """
        SELECT
            c.type "cluster_type",
            sc.roles,
            count(1) hosts,
            sum(f.cpu_guarantee) cpu_cores,
            sum(f.memory_guarantee) memory_size,
            sum(h.space_limit) disk_size
        FROM dbaas.hosts h
        JOIN dbaas.flavors f ON (f.id = h.flavor)
        JOIN dbaas.geo g USING (geo_id)
        JOIN dbaas.subclusters sc USING (subcid)
        JOIN dbaas.clusters c USING (cid)
        WHERE c.status::text != ALL('{STOPPED,DELETED,METADATA-DELETED,METADATA-DELETING,PURGED,PURGING}'::text[])
        {% if cluster_types %}
          AND c.type = ANY(%(cluster_types)s::dbaas.cluster_type[])
        {% endif %}
        {% if role %}
          AND %(role)s = ANY(sc.roles::text[])
        {% endif %}
        {% if zones %}
          AND g.name = ANY(%(zones)s)
        {% endif %}
        GROUP BY ROLLUP (c.type, sc.roles)
        ORDER BY c.type, sc.roles
        """
    return db_query(ctx, 'metadb', query, cluster_types=to_db_cluster_types(cluster_types), role=role, zones=zones)


def delete_hosts(ctx, cluster_id, hosts, revision) -> None:
    """
    Delete hosts from metadb.
    """
    fqdns = [host['public_fqdn'] for host in hosts]
    db_query(
        ctx,
        'metadb',
        """
             SELECT code.delete_hosts(
                i_cid   => %(cluster_id)s,
                i_fqdns => %(fqdns)s,
                i_rev   => %(rev)s
             )
             """,
        cluster_id=cluster_id,
        fqdns=fqdns,
        rev=revision,
    )


def update_host(ctx, hostname, *, new_hostname=None, zone=None, disk_size=None):
    """
    Update host in metadb.
    """
    result = db_query(
        ctx,
        'metadb',
        """
        UPDATE dbaas.hosts
        SET
        {% if disk_size %}
            space_limit = %(disk_size)s,
        {% endif %}
        {% if zone_id %}
            geo_id = %(zone_id)s,
        {% endif %}
        {% if new_hostname %}
            fqdn = %(new_hostname)s
        {% else %}
            fqdn = fqdn
        {% endif %}
        WHERE fqdn = %(hostname)s
        RETURNING fqdn
        """,
        hostname=hostname,
        new_hostname=new_hostname,
        zone_id=zone['id'] if zone else None,
        disk_size=disk_size,
        fetch_single=True,
    )

    if not result:
        raise HostNotFound(hostname)


def resetup_host_task(
    ctx,
    cluster,
    hostname,
    resetup_action,
    resetup_from,
    ignore_hostnames=None,
    preserve=False,
    health_check=True,
    hidden=False,
    timeout=None,
):
    """
    Create task to resetup host.
    """
    db_cluster_type = cluster['type']
    if cluster['status'] == STOPPED_CLUSTER_STATUS:
        task_type = f'{db_cluster_type}_offline_resetup'
    else:
        task_type = f'{db_cluster_type}_online_resetup'

    task_args = {
        "fqdn": to_overlay_fqdn(ctx, hostname),
        "resetup_action": resetup_action,
        "preserve_if_possible": preserve,
        "ignore_hosts": ignore_hostnames or [],
        "disable_health_check": not health_check,
        "lock_is_already_taken": False,
        "try_save_disks": False,
        "cid": cluster['id'],
        "resetup_from": resetup_from,
    }

    return create_task(
        ctx,
        cluster_id=cluster['id'],
        task_type=task_type,
        operation_type=task_type,
        task_args=task_args,
        hidden=hidden,
        timeout=timeout,
        revision=cluster['revision'],
    )


def delete_host_task(ctx, cluster, host, args=None, health_check=None, hidden=None, timeout=None):
    """
    Create task to delete host.
    """
    hostname = host['public_fqdn']
    cluster_type = to_cluster_type(db_cluster_type=cluster['type'])
    task_type = f'{cluster_type}_host_delete'

    if not args:
        args = {}
    args['host'] = {
        'fqdn': hostname,
        'subcid': host['subcluster_id'],
        'shard_id': host['shard_id'],
        'vtype': host['vtype'],
        'vtype_id': host['vtype_id'],
    }
    args['disable_health_check'] = not health_check

    return create_task(
        ctx,
        cluster_id=cluster['id'],
        task_type=task_type,
        operation_type=task_type,
        task_args=args,
        metadata={
            'host_names': [hostname],
        },
        hidden=hidden,
        timeout=timeout,
        revision=cluster['revision'],
    )
