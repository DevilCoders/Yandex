from collections import OrderedDict

from click import argument, group, option, pass_context
from cloud.mdb.cli.common.formatting import format_bytes, print_response
from cloud.mdb.cli.common.parameters import ListParamType
from cloud.mdb.cli.common.utils import ensure_no_unsupported_options
from cloud.mdb.cli.dbaas.internal.intapi import ClusterType, perform_operation_rest, rest_request
from cloud.mdb.cli.dbaas.internal.metadb.cluster import cluster_lock, get_cluster_type_by_id
from cloud.mdb.cli.dbaas.internal.metadb.host import get_hosts
from cloud.mdb.cli.dbaas.internal.metadb.shard import get_shard, get_shards, update_shard_name
from cloud.mdb.cli.dbaas.internal.utils import cluster_status_options, DELETED_CLUSTER_STATUSES, format_hosts


@group('shard')
def shard_group():
    """Shard management commands."""
    pass


@shard_group.command('get')
@argument('shard_id', metavar='ID')
@option('--api', 'api_mode', is_flag=True, help='Perform request through REST API rather than DB query.')
@pass_context
def get_shard_command(ctx, api_mode, shard_id):
    """Get shard.

    For getting folder by related object, ID argument accepts hostname
    in addition to shard ID."""
    command_impl = _get_shard_api if api_mode else _get_shard_db
    command_impl(ctx, get_shard(ctx, untyped_id=shard_id))


def _get_shard_api(ctx, shard):
    cluster_id = shard['cluster_id']
    shard_name = shard['name']
    response = rest_request(
        ctx, 'GET', get_cluster_type_by_id(ctx, cluster_id), f'clusters/{cluster_id}/shards/{shard_name}'
    )
    print_response(ctx, response)


def _get_shard_db(ctx, shard):
    hosts = get_hosts(ctx, shard_id=shard['id'])

    result = OrderedDict((key, value) for key, value in shard.items() if key not in 'pillar')
    result['zones'] = ','.join(set(h['zone'] for h in hosts))
    result['flavor'] = hosts[0]['flavor']
    result['disk_type'] = hosts[0]['disk_type']
    result['disk_size'] = format_bytes(hosts[0]['disk_size'])
    result['hosts'] = format_hosts(hosts)

    print_response(ctx, result)


@shard_group.command('list')
@argument('untyped_ids', metavar='IDS', required=False, type=ListParamType())
@option('--api', 'api_mode', is_flag=True, help='Perform request through REST API rather than DB query.')
@option('-f', '--folder', 'folder_id', help='Filter objects to output by folder ID.')
@option('-c', '--cluster', 'cluster_id', help='Filter objects to output by cluster ID.')
@option(
    '--cluster-type',
    '--cluster-types',
    'cluster_types',
    type=ListParamType(ClusterType()),
    help='Filter objects to output by one or several cluster types. Multiple values can be specified through a comma.',
)
@option('-e', '--env', '--cluster-env', 'cluster_env', help='Filter objects to output by cluster environment.')
@cluster_status_options()
@option(
    '--sc',
    '--subcluster',
    '--subclusters',
    'subcluster_ids',
    type=ListParamType(),
    help='Filter objects to output by one or several subclusters. Multiple values can be specified through a comma.',
)
@option('-r', '--role', help='Filter objects to output by subcluster role.')
@option(
    '-S',
    '--shard',
    '--shards',
    'shard_ids',
    type=ListParamType(),
    help='Filter objects to output by one or several shards. Multiple values can be specified through a comma.',
)
@option(
    '--xshard',
    '--exclude-shard',
    '--exclude-shards',
    'exclude_shard_ids',
    type=ListParamType(),
    help='Filter objects to not output by one or several shards. Multiple values can be specified through a comma.',
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
    '--pillar',
    '--pillar-match',
    'pillar_filter',
    metavar='JSONPATH_EXPRESSION',
    help='Filter objects by matching pillar against the specified jsonpath expression. The matching is performed'
    ' using jsonb_path_match PostgreSQL function.',
)
@option('-l', '--limit', type=int, help='Limit the max number of objects in the output.')
@option('-q', '--quiet', is_flag=True, help='Output only shard IDs.')
@option('-s', '--separator', help='Value separator for quiet mode.')
@pass_context
def list_shards_command(ctx, api_mode, **kwargs):
    """List shards.

    For getting shards by related objects, IDs argument accepts cluster IDs, subcluster IDs and hostnames
    in addition to shard IDs. NOTE: applicable for DB mode only."""
    command_impl = _list_shards_api if api_mode else _list_shards_db
    command_impl(ctx, **kwargs)


def _list_shards_api(ctx, cluster_id, quiet, separator, **kwargs):
    ensure_no_unsupported_options(
        ctx,
        kwargs,
        {
            'untyped_ids': 'IDS',
            'cluster_type': '--cluster-type',
            'role': '--role',
            'env': '--env',
            'subcluster_ids': '--subcluster',
            'hostnames': '--hosts',
            'limit': '--limit',
        },
        '{0} option is not supported in api mode.',
    )

    if not cluster_id:
        ctx.fail('--cluster option is required in api mode.')

    response = rest_request(ctx, 'GET', get_cluster_type_by_id(ctx, cluster_id), f'clusters/{cluster_id}/shards')
    print_response(ctx, response['shards'], quiet=quiet, separator=separator)


def _list_shards_db(
    ctx,
    untyped_ids,
    cluster_id,
    subcluster_ids,
    shard_ids,
    hostnames,
    cluster_statuses,
    exclude_cluster_statuses,
    limit,
    quiet,
    separator,
    **kwargs,
):
    if limit is None:
        limit = 1000

    if exclude_cluster_statuses is None and not any(
        (cluster_statuses, untyped_ids, cluster_id, subcluster_ids, shard_ids, hostnames)
    ):
        exclude_cluster_statuses = DELETED_CLUSTER_STATUSES

    shards = get_shards(
        ctx,
        untyped_ids=untyped_ids,
        cluster_id=cluster_id,
        subcluster_ids=subcluster_ids,
        shard_ids=shard_ids,
        hostnames=hostnames,
        cluster_statuses=cluster_statuses,
        exclude_cluster_statuses=exclude_cluster_statuses,
        limit=limit,
        **kwargs,
    )
    print_response(
        ctx,
        shards,
        default_format='table',
        fields=['id', 'name', 'roles', 'created_at', 'cluster_id'],
        quiet=quiet,
        separator=separator,
    )


@shard_group.command('create')
@argument('cluster_id', metavar='CLUSTER')
@argument('name', required=False)
@option('--hidden', is_flag=True, help='Mark underlying task as hidden.')
@pass_context
def create_shard_command(ctx, cluster_id, name, hidden):
    """List shards."""
    if not name:
        shard_count = len(get_shards(ctx, cluster_id=cluster_id))
        name = f'shard{shard_count + 1}'

    cluster_type = get_cluster_type_by_id(ctx, cluster_id)
    perform_operation_rest(
        ctx,
        'POST',
        cluster_type,
        f'clusters/{cluster_id}/shards',
        hidden=hidden,
        data={
            'shardName': name,
        },
    )


@shard_group.command('rename')
@argument('shard_id', metavar='SHARD')
@argument('new_name')
@pass_context
def rename_shard_command(ctx, shard_id, new_name):
    """Update shard name in metadb."""
    shard = get_shard(ctx, shard_id=shard_id)
    with cluster_lock(ctx, shard['cluster_id']):
        update_shard_name(ctx, shard_id, new_name)

    print(f'Shard name was updated from "{shard["name"]}" to "{new_name}"')


@shard_group.command('delete')
@argument('cluster_id', metavar='CLUSTER')
@argument('name')
@option('--hidden', is_flag=True, help='Mark underlying task as hidden.')
@pass_context
def delete_shard_command(ctx, cluster_id, name, hidden):
    """Delete shard."""
    cluster_type = get_cluster_type_by_id(ctx, cluster_id)
    perform_operation_rest(ctx, 'DELETE', cluster_type, f'clusters/{cluster_id}/shards/{name}', hidden=hidden)
