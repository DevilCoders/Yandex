import sys
from collections import OrderedDict

from click import argument, Choice, ClickException, group, option, pass_context

from cloud.mdb.cli.common.formatting import (
    format_bytes,
    print_response,
)
from cloud.mdb.cli.common.parameters import ListParamType, BytesParamType
from cloud.mdb.cli.common.prompts import confirm_dangerous_action
from cloud.mdb.cli.common.utils import ensure_no_unsupported_options
from cloud.mdb.cli.dbaas.internal import conductor, juggler
from cloud.mdb.cli.dbaas.internal.common import to_overlay_fqdn
from cloud.mdb.cli.dbaas.internal.config import get_vtype
from cloud.mdb.cli.dbaas.internal.dbm.porto_container import delete_porto_container, find_porto_container
from cloud.mdb.cli.dbaas.internal.deploy import delete_minion
from cloud.mdb.cli.dbaas.internal.intapi import ClusterType, rest_request
from cloud.mdb.cli.dbaas.internal.metadb.cloud import update_cloud_used_resources
from cloud.mdb.cli.dbaas.internal.metadb.cluster import (
    cluster_lock,
    get_cluster,
    get_cluster_type,
    get_cluster_type_by_id,
)
from cloud.mdb.cli.dbaas.internal.metadb.common import to_cluster_type
from cloud.mdb.cli.dbaas.internal.metadb.host import (
    delete_host_task,
    delete_hosts,
    filter_hosts,
    get_host,
    get_host_stats,
    get_hosts,
    reload_host,
    resetup_host_task,
    update_host,
)
from cloud.mdb.cli.dbaas.internal.metadb.pillar import delete_pillar, get_pillar, update_pillar
from cloud.mdb.cli.dbaas.internal.metadb.subcluster import get_subcluster
from cloud.mdb.cli.dbaas.internal.metadb.task import SequentialTaskExecuter
from cloud.mdb.cli.dbaas.internal.metadb.zone import get_zone
from cloud.mdb.cli.dbaas.internal.utils import (
    cluster_status_options,
    DELETED_CLUSTER_STATUSES,
    format_hosts,
    format_references,
    RUNNING_CLUSTER_STATUS,
    STOPPED_CLUSTER_STATUS,
)

FIELD_FORMATTERS = {
    'cluster_type': to_cluster_type,
    'memory_size': format_bytes,
    'disk_size': format_bytes,
}


@group('host')
def host_group():
    """Host management commands."""
    pass


@host_group.command('get')
@argument('untyped_id', metavar='HOST')
@pass_context
def get_host_command(ctx, untyped_id):
    """Get host.

    For getting host by related object, ID argument accepts compute instance ID,
    cluster ID, subcluster ID or shard ID in addition to hostname."""
    try:
        host = get_host(ctx, hostname=untyped_id)
    except Exception:
        host = get_host(ctx, untyped_id=untyped_id)
    cluster = get_cluster(ctx, host['cluster_id'])

    result = OrderedDict((key, value) for key, value in host.items())
    if host['shard_id']:
        result['shard_hosts'] = format_hosts(get_hosts(ctx, shard_id=host['shard_id']))
    else:
        result['subcluster_hosts'] = format_hosts(get_hosts(ctx, subcluster_id=host['subcluster_id']))
    result['references'] = _format_references(ctx, cluster, host)

    print_response(ctx, result, field_formatters=FIELD_FORMATTERS, ignored_fields=['disk_quota_type'])


def _format_references(ctx, cluster, host):
    macros = {
        'folder_id': cluster['folder_id'],
        'cluster_type': get_cluster_type(cluster),
        'cluster_id': cluster['id'],
        'host': host['public_fqdn'],
        'vtype_id': host['vtype_id'],
    }
    return format_references(ctx, 'host_references', macros)


@host_group.command('list')
@argument('untyped_ids', metavar='IDS', required=False, type=ListParamType())
@option('--api', 'api_mode', is_flag=True, help='Perform request through REST API rather than DB query.')
@option('-f', '--folder', 'folder_id', help='Filter objects to output by folder.')
@option(
    '-c',
    '--cluster',
    '--clusters',
    'cluster_ids',
    type=ListParamType(),
    help='Filter objects to output by one or several clusters. Multiple values can be specified through a comma.',
)
@option(
    '--xcluster',
    '--exclude-cluster',
    '--exclude-clusters',
    'exclude_cluster_ids',
    type=ListParamType(),
    help='Filter objects to not output by one or several clusters. Multiple values can be specified through a comma.',
)
@option(
    '--sc',
    '--subcluster',
    '--subclusters',
    'subcluster_ids',
    type=ListParamType(),
    help='Filter objects to output by one or several subclusters. Multiple values can be specified through a comma.',
)
@option(
    '-S',
    '--shard',
    '--shards',
    'shard_ids',
    type=ListParamType(),
    help='Filter objects to output by one or several shards. Multiple values can be specified through a comma.',
)
@option(
    '-H',
    '--host',
    '--hosts',
    'hostnames',
    type=ListParamType(),
    help='Filter objects to output by one or several host names. Multiple values can be specified through a comma.',
)
@option(
    '--vtype-id',
    '--vtype-ids',
    'vtype_ids',
    type=ListParamType(),
    help='Filter objects to output by one or several vtype IDs. Multiple values can be specified through a comma.',
)
@option(
    '--task',
    '--tasks',
    'task_ids',
    type=ListParamType(),
    help='Filter objects to output by one or several tasks. Multiple values can be specified through a comma.',
)
@option(
    '--cluster-type',
    '--cluster-types',
    'cluster_types',
    type=ListParamType(ClusterType()),
    help='Filter objects to output by one or several cluster types. Multiple values can be specified through a comma.',
)
@option('-e', '--env', '--cluster-env', 'cluster_env', help='Filter objects to output by cluster environment.')
@cluster_status_options()
@option('-r', '--role', help='Filter objects to output by subcluster role.')
@option('--flavor', '--resource-preset', 'flavor', help='Filter objects to output by resource preset.')
@option(
    '-G',
    '--generation',
    '--generations',
    'generations',
    type=ListParamType(int),
    help='Filter objects to output by one or several resource preset generations. Multiple values can be specified through a comma.',
)
@option('--disk-type', help='Filter objects to output by disk type.')
@option(
    '-g',
    '--geo',
    '--zone',
    '--zones',
    'zones',
    type=ListParamType(),
    help='Filter objects to output by one or several availability zones. Multiple values can be specified through a comma.',
)
@option('-l', '--limit', type=int, help='Limit the max number of objects in the output.')
@option('-q', '--quiet', is_flag=True, help='Output only host names.')
@option('-s', '--separator', help='Value separator for quiet mode.')
@pass_context
def list_hosts_command(ctx, api_mode, **kwargs):
    """List hosts.

    For getting hosts by related objects, IDs argument accepts cluster IDs, subcluster IDs and shard IDs
    in addition to hostnames. NOTE: applicable for DB mode only."""
    command_impl = _list_hosts_api if api_mode else _list_hosts_db
    command_impl(ctx, **kwargs)


def _list_hosts_api(ctx, cluster_ids, quiet, separator, **kwargs):
    ensure_no_unsupported_options(
        ctx,
        kwargs,
        {
            'untyped_ids': 'IDS',
            'env': '--env',
            'flavor': '--flavor',
            'disk_type': '--disk-type',
            'zone': '--zone',
            'status': '--status',
            'exclude_status': '--exclude-status',
            'exclude_cluster_ids': '--exclude-clusters',
            'subcluster_ids': '--subclusters',
            'shard_ids': '--shards',
            'hostnames': '--hosts',
            'vtype_ids': '--vtype-ids',
            'task_ids': '--tasks',
            'cluster_types': '--cluster-types',
            'role': '--role',
            'limit': '--limit',
        },
        '{0} option is not supported in api mode.',
    )

    def _table_formatter(host):
        result = OrderedDict()
        result['fqdn'] = host['name']
        if 'type' in host:
            result['type'] = host['type']
        if 'role' in host:
            result['role'] = host['role']
        result['zone'] = host['zoneId']
        result['health'] = host['health']
        result['flavor'] = host['resources']['resourcePresetId']
        result['disk type'] = host['resources'].get('diskTypeId', None)
        result['disk size'] = format_bytes(host['resources']['diskSize'])
        return result

    if not cluster_ids:
        ctx.fail('--cluster option is required in api mode.')
    if len(cluster_ids) > 1:
        ctx.fail('Passing multiple values for --cluster option is not support in api mode.')
    cluster_id = cluster_ids[0]

    response = rest_request(ctx, 'GET', get_cluster_type_by_id(ctx, cluster_id), f'clusters/{cluster_id}/hosts')
    print_response(
        ctx,
        response['hosts'],
        default_format='table',
        table_formatter=_table_formatter,
        quiet=quiet,
        id_key='name',
        separator=separator,
    )


def _list_hosts_db(
    ctx,
    untyped_ids,
    cluster_ids,
    subcluster_ids,
    shard_ids,
    hostnames,
    vtype_ids,
    task_ids,
    cluster_statuses,
    exclude_cluster_statuses,
    limit,
    quiet,
    separator,
    **kwargs,
):
    def _table_formatter(host):
        return OrderedDict(
            (
                ('fqdn', host['fqdn']),
                ('vtype id', host['vtype_id']),
                ('roles', host['roles']),
                ('zone', host['zone']),
                ('flavor', host['flavor']),
                ('disk type', host['disk_type']),
                ('disk size', host['disk_size']),
                ('shard id', host['shard_id']),
            )
        )

    if limit is None:
        limit = 1000

    if exclude_cluster_statuses is None and not any(
        (cluster_statuses, untyped_ids, cluster_ids, subcluster_ids, shard_ids, hostnames, vtype_ids, task_ids)
    ):
        exclude_cluster_statuses = DELETED_CLUSTER_STATUSES

    hosts = get_hosts(
        ctx,
        untyped_ids=untyped_ids,
        cluster_ids=cluster_ids,
        subcluster_ids=subcluster_ids,
        shard_ids=shard_ids,
        hostnames=hostnames,
        vtype_ids=vtype_ids,
        task_ids=task_ids,
        cluster_statuses=cluster_statuses,
        exclude_cluster_statuses=exclude_cluster_statuses,
        limit=limit,
        **kwargs,
    )
    print_response(
        ctx,
        hosts,
        default_format='table',
        field_formatters=FIELD_FORMATTERS,
        table_formatter=_table_formatter,
        quiet=quiet,
        id_key='fqdn',
        separator=separator,
    )


@host_group.command('update-target-resources')
@argument('hostnames', metavar='HOSTS', type=ListParamType())
@option('--disk-size', required=True, type=BytesParamType())
@option('--force', is_flag=True, help='Suppress confirmation prompts and sanity checks.')
@pass_context
def update_targer_resources_command(ctx, hostnames, disk_size, force):
    """Update host resources in metadb."""
    confirm_dangerous_action('You are going to perform potentially dangerous action.', force)

    for host in _get_hosts(ctx, hostnames):
        _update_host_resources(ctx, host, disk_size)


def _update_host_resources(ctx, host, disk_size):
    cluster_id = host['cluster_id']
    with cluster_lock(ctx, cluster_id):
        host = reload_host(ctx, host)
        hostname = host['public_fqdn']
        cluster = get_cluster(ctx, cluster_id)

        disk_size_diff = disk_size - host['disk_size']

        update_cloud_used_resources(
            ctx,
            cluster['cloud_id'],
            add_disk_space=disk_size_diff,
            disk_quota_type=host['disk_quota_type'],
        )

        update_host(ctx, hostname=hostname, disk_size=disk_size)

        print(f'Host {hostname} updated.')


@host_group.command('stats')
@option(
    '-t',
    '--cluster-type',
    '--cluster-types',
    'cluster_types',
    type=ListParamType(ClusterType()),
    help='Count hosts with the specified cluster types only.',
)
@option('-r', '--role', help='Count hosts with the specified subcluster role only.')
@option(
    '-g',
    '--geo',
    '--zone',
    '--zones',
    'zones',
    type=ListParamType(),
    help='Count hosts with the specified availability zones only.',
)
@pass_context
def get_host_stats_command(ctx, **kwargs):
    """Ger statistics on hosts."""
    stats = get_host_stats(ctx, **kwargs)
    print_response(
        ctx,
        stats,
        default_format='table',
        field_formatters=FIELD_FORMATTERS,
        fields=['cluster_type', 'roles', 'hosts', 'cpu_cores', 'memory_size', 'disk_size'],
    )


@host_group.command('resetup')
@argument('hostnames', metavar='HOSTS', type=ListParamType())
@option('--action', type=Choice(['readd', 'restore']), default='readd')
@option('--new-zone', '--new-geo', 'new_zone_name')
@option('--new-hostname', 'new_hostname')
@option('--preserve-hostname', is_flag=True)
@option('--ignore-hosts', 'ignore_hostnames', type=ListParamType())
@option('--ignore-zone', '--ignore-geo', 'ignore_zone_name')
@option(
    '--health/--no-health',
    '--health-check/--no-health-check',
    'health_check',
    default=True,
    help='Enable/Disable health checks for resetup task.',
)
@option(
    '--hidden', is_flag=True, help='Mark creating resetup task as hidden. Hidden tasks are not visible to end users.'
)
@option('--timeout', default='24 hours', help='Task timeout.')
@option('--force', is_flag=True, help='Suppress confirmation prompts and sanity checks.')
@option('-k', '--keep-going', is_flag=True, default=False, help='Do not stop on the first failed task.')
@pass_context
def resetup_command(
    ctx,
    hostnames,
    action,
    new_zone_name,
    new_hostname,
    preserve_hostname,
    ignore_hostnames,
    ignore_zone_name,
    health_check,
    hidden,
    timeout,
    force,
    keep_going,
):
    """Resetup one or several hosts."""
    new_zone = get_zone(ctx, new_zone_name) if new_zone_name else None

    if new_hostname and not new_zone:
        raise ClickException('Option --new-hostname cannot be used without option --new-zone.')

    if new_hostname and preserve_hostname:
        raise ClickException('Options --new-hostname and --preserve-hostname cannot be used together.')

    if new_hostname and len(hostnames) > 1:
        raise ClickException('Option --new-hostname cannot be used when resetup is performing for multiple hosts.')

    confirm_dangerous_action('You are going to perform potentially dangerous action.', force)

    executer = SequentialTaskExecuter(ctx, keep_going=keep_going)
    for host in _get_hosts(ctx, hostnames, keep_going=keep_going):
        executer.submit(
            _resetup_host,
            ctx,
            host=host,
            action=action,
            new_zone=new_zone,
            new_hostname=new_hostname,
            preserve_hostname=preserve_hostname,
            ignore_hostnames=ignore_hostnames,
            ignore_zone_name=ignore_zone_name,
            health_check=health_check,
            hidden=hidden,
            timeout=timeout,
            force=force,
        )

    completed, failed = executer.run()

    if failed and len(hostnames) > 1:
        ctx.fail(f'Failed tasks: {", ".join(failed)}')


def _resetup_host(
    ctx,
    host,
    action,
    new_zone,
    new_hostname,
    preserve_hostname,
    ignore_hostnames,
    ignore_zone_name,
    health_check,
    hidden,
    timeout,
    force,
):
    hostname = host['public_fqdn']
    cluster_id = host['cluster_id']
    with cluster_lock(ctx, cluster_id):
        cluster = get_cluster(ctx, cluster_id)

        ignore_hostnames = [to_overlay_fqdn(ctx, hostname) for hostname in ignore_hostnames or []]
        if ignore_zone_name:
            ignore_hosts = get_hosts(ctx, cluster_id=cluster_id, zones=[ignore_zone_name])
            ignore_hostnames += [h['public_fqdn'] for h in ignore_hosts if h['fqdn'] != host['fqdn']]

        if cluster['status'] not in (RUNNING_CLUSTER_STATUS, STOPPED_CLUSTER_STATUS):
            raise ClickException(
                f'Host {hostname} cannot be resetuped: cluster status must be "{RUNNING_CLUSTER_STATUS}"'
                f' or "{STOPPED_CLUSTER_STATUS}".'
            )

        if not force and juggler.get_status(ctx, hostname, 'UNREACHABLE', None) == 'OK':
            raise ClickException(f'Host "{hostname}" is up and running. Resetup is not required.')

        preserve = False
        if get_vtype(ctx) == 'porto':
            container = find_porto_container(ctx, hostname)
            if container:
                resetup_from = container['dom0']
            else:
                resetup_from = None
                preserve = True
            if new_zone or new_hostname:
                hostname = _replace_host_zone(
                    ctx, host, new_zone, new_hostname=new_hostname, preserve_hostname=preserve_hostname
                )
                preserve = True
        else:
            if new_zone:
                raise ClickException('Option --new-zone / --new-geo is supported only in porto.')
            resetup_from = host['vtype_id']

        return resetup_host_task(
            ctx,
            cluster=cluster,
            hostname=hostname,
            resetup_action=action,
            resetup_from=resetup_from,
            preserve=preserve,
            ignore_hostnames=ignore_hostnames,
            health_check=health_check,
            hidden=hidden,
            timeout=timeout,
        )


@host_group.command('delete')
@argument('hostnames', metavar='HOSTS', type=ListParamType())
@option(
    '--health/--no-health',
    '--health-check/--no-health-check',
    'health_check',
    default=True,
    help='Enable/Disable health checks for resetup task.',
)
@option('--hidden', is_flag=True, help='Mark underlying task as hidden.')
@option('--timeout', default='4 hours', help='Task timeout.')
@option('--force', is_flag=True, help='Suppress confirmation prompts and sanity checks.')
@option('-k', '--keep-going', is_flag=True, default=False, help='Do not stop on the first failed task.')
@pass_context
def delete_host_command(ctx, hostnames, health_check, hidden, timeout, force, keep_going):
    """Delete one or several hosts."""
    confirm_dangerous_action('You are going to perform potentially dangerous action.', force)

    executer = SequentialTaskExecuter(ctx, keep_going=keep_going)
    for host in _get_hosts(ctx, hostnames, keep_going=keep_going):
        executer.submit(_delete_host, ctx, host=host, health_check=health_check, hidden=hidden, timeout=timeout)

    completed, failed = executer.run()

    if failed and len(hostnames) > 1:
        ctx.fail(f'Failed tasks: {", ".join(failed)}')


def _delete_host(ctx, host, health_check, hidden, timeout):
    hostname = host['public_fqdn']
    shard_id = host['shard_id']
    cluster_id = host['cluster_id']
    subcluster_id = host['subcluster_id']
    with cluster_lock(ctx, cluster_id):
        host = reload_host(ctx, host)
        cluster = get_cluster(ctx, cluster_id)
        cluster_type = get_cluster_type(cluster)
        cluster_hosts = get_hosts(ctx, cluster_id=cluster_id)

        if shard_id:
            if len(filter_hosts(cluster_hosts, shard_id=shard_id)) < 2:
                raise ClickException(f'Host {hostname} cannot be deleted as it is the last host in shard {shard_id}.')
        else:
            if len(filter_hosts(cluster_hosts, subcluster_id=subcluster_id)) < 2:
                raise ClickException(
                    f'Host {hostname} cannot be deleted as it is the last host in subcluster {subcluster_id}.'
                )

        if cluster['status'] not in (RUNNING_CLUSTER_STATUS, STOPPED_CLUSTER_STATUS):
            raise ClickException(
                f'Host {hostname} cannot be deleted: cluster status must be "{RUNNING_CLUSTER_STATUS}"'
                f' or "{STOPPED_CLUSTER_STATUS}".'
            )

        if cluster_type == 'clickhouse':
            task_args = _handle_clickhouse_cluster_host_deletion(ctx, host=host, cluster_hosts=cluster_hosts)
        elif cluster_type == 'kafka':
            task_args = _handle_kafka_cluster_host_deletion(ctx, host=host)
        else:
            raise ClickException(f'Host {hostname} cannot be deleted: not supported for {cluster_type} clusters.')

        update_cloud_used_resources(
            ctx,
            cluster['cloud_id'],
            add_cpu=-host['cpu_cores'],
            add_gpu=-host['gpu_cores'],
            add_memory=-host['memory_size'],
            add_disk_space=-host['disk_size'],
            disk_quota_type=host['disk_quota_type'],
        )

        delete_hosts(ctx, cluster_id=cluster_id, hosts=[host], revision=cluster['revision'])

        return delete_host_task(
            ctx, cluster=cluster, host=host, args=task_args, health_check=health_check, hidden=hidden, timeout=timeout
        )


def _handle_clickhouse_cluster_host_deletion(ctx, *, host, cluster_hosts):
    hostname = host['public_fqdn']
    cluster_id = host['cluster_id']
    subcluster_id = host['subcluster_id']
    ch_subcluster = get_subcluster(ctx, cluster_id=cluster_id, role='clickhouse_cluster')
    ch_pillar = ch_subcluster['pillar']

    if 'zk' in host['roles']:
        if len(filter_hosts(cluster_hosts, role='zk')) < 4:
            raise ClickException(f'Host {hostname} cannot be deleted: ZooKeeper must have at least 3 hosts. ')
        zk_pillar = get_pillar(ctx, subcluster_id=subcluster_id)
        zk_nodes = zk_pillar['data']['zk']['nodes']
        zk_nodes.pop(host['public_fqdn'], None)
        update_pillar(ctx, zk_pillar, subcluster_id=subcluster_id)
    else:
        if hostname in (ch_pillar['data']['clickhouse'].get('zk_hosts') or []):
            raise ClickException(
                f'Host {hostname} cannot be deleted: deletion of ClickHouse Keeper hosts is not supported.'
            )

    task_args = {}
    zk_hostnames = [h['public_fqdn'] for h in filter_hosts(cluster_hosts, role='zk')]
    if not zk_hostnames:
        zk_hostnames = ch_pillar['data']['clickhouse'].get('zk_hosts') or []
    task_args['zk_hosts'] = ','.join(f'{zk_hostname}:2181' for zk_hostname in zk_hostnames)

    return task_args


def _handle_kafka_cluster_host_deletion(ctx, *, host):
    hostname = host['public_fqdn']
    cluster_id = host['cluster_id']

    if 'kafka_cluster' in host['roles']:
        kafka_pillar = get_pillar(ctx, cluster_id=cluster_id)
        nodes = kafka_pillar['data']['kafka']['nodes']
        nodes.pop(host['public_fqdn'], None)
        update_pillar(ctx, kafka_pillar, cluster_id=cluster_id)
    else:
        raise ClickException(f'Host {hostname} cannot be deleted: deletion of Kafka ZK hosts is not supported.')

    return {}


def _replace_host_zone(ctx, host, new_zone, new_hostname=None, preserve_hostname=False):
    hostname = host['public_fqdn']
    subcluster_id = host['subcluster_id']

    if not new_hostname:
        if preserve_hostname:
            new_hostname = hostname
        else:
            new_hostname = hostname.replace(host['zone'], new_zone['name'], 1)

    if not preserve_hostname:
        delete_pillar(ctx, host=hostname)

    update_host(ctx, hostname, new_hostname=new_hostname, zone=new_zone)

    if not preserve_hostname:
        if 'zk' in host['roles']:
            if host['cluster_type'] == 'kafka_cluster':
                pillar_cluster_id = host['cluster_id']
                pillar_subcluster_id = None
            else:
                pillar_cluster_id = None
                pillar_subcluster_id = subcluster_id

            zk_pillar = get_pillar(ctx, cluster_id=pillar_cluster_id, subcluster_id=pillar_subcluster_id)
            zk_nodes = zk_pillar['data']['zk']['nodes']
            node_id = zk_nodes.pop(hostname)
            zk_nodes[new_hostname] = node_id
            update_pillar(ctx, zk_pillar, cluster_id=pillar_cluster_id, subcluster_id=pillar_subcluster_id)
        if 'clickhouse_cluster' in host['roles']:
            ch_pillar = get_pillar(ctx, subcluster_id=subcluster_id)

            keeper_hosts = ch_pillar['data']['clickhouse'].get('keeper_hosts')
            if keeper_hosts:
                new_server_id = max(keeper_hosts.values()) + 1

                keeper_hosts.pop(hostname, None)
                if new_hostname not in keeper_hosts:
                    keeper_hosts[new_hostname] = new_server_id

                update_pillar(ctx, ch_pillar, subcluster_id=subcluster_id)

            zk_hosts = ch_pillar['data']['clickhouse'].get('zk_hosts')
            if zk_hosts and not keeper_hosts:
                for i, zk_host in enumerate(zk_hosts):
                    if hostname == zk_host:
                        zk_hosts[i] = new_hostname

                update_pillar(ctx, ch_pillar, subcluster_id=subcluster_id)
        if 'kafka_cluster' in host['roles']:
            cluster_pillar = get_pillar(ctx, cluster_id=host['cluster_id'])
            kafka_nodes = cluster_pillar['data']['kafka']['nodes']
            node = kafka_nodes.pop(hostname)
            node['fqdn'] = new_hostname
            node['rack'] = new_zone
            kafka_nodes[new_hostname] = node
            update_pillar(ctx, cluster_pillar, subcluster_id=subcluster_id)

    if not preserve_hostname:
        conductor.delete_host(ctx, hostname, suppress_errors=True)

    delete_porto_container(ctx, hostname)

    if not preserve_hostname:
        delete_minion(ctx, hostname, suppress_errors=True)

    return new_hostname


def _get_hosts(ctx, hostnames, keep_going=False):
    hosts = get_hosts(ctx, hostnames=hostnames)
    if not hosts:
        raise ClickException('No hosts found.')

    if len(hostnames) != len(hosts):
        not_found_ids = set(hostnames)
        for host in hosts:
            not_found_ids.discard(host['fqdn'])
            not_found_ids.discard(host['public_fqdn'])

        if len(not_found_ids) == 1:
            message = f'Host "{not_found_ids.pop()}" not found.'
        else:
            message = f'Multiple hosts were not found: {",".join(not_found_ids)}'

        if keep_going:
            print(message, file=sys.stderr)
        else:
            raise ClickException(message)

    return hosts
