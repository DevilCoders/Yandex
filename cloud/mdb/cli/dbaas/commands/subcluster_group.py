from click import argument, group, option, pass_context

from cloud.mdb.cli.common.formatting import format_bytes, print_response
from cloud.mdb.cli.common.parameters import FieldsParamType, ListParamType
from cloud.mdb.cli.dbaas.internal.intapi import ClusterType
from cloud.mdb.cli.dbaas.internal.metadb.flavor import FlavorType
from cloud.mdb.cli.dbaas.internal.metadb.subcluster import get_subcluster, get_subclusters
from cloud.mdb.cli.dbaas.internal.utils import cluster_status_options, DELETED_CLUSTER_STATUSES


FIELD_FORMATTERS = {
    'memory_size': format_bytes,
    'disk_size': format_bytes,
}


@group('subcluster')
def subcluster_group():
    """Subcluster management commands."""
    pass


@subcluster_group.command('get')
@argument('untyped_id', metavar='ID')
@pass_context
def get_cluster_command(ctx, untyped_id):
    """Get subcluster.

    For getting subcluster by related object, ID argument accepts shard ID and hostanme in addition to subcluster ID.
    """
    subcluster = get_subcluster(ctx, untyped_id=untyped_id, with_stats=True)
    print_response(ctx, subcluster, field_formatters=FIELD_FORMATTERS, ignored_fields=['pillar'])


@subcluster_group.command('list')
@argument('untyped_ids', metavar='IDS', required=False, type=ListParamType())
@option('-f', '--folder', 'folder_id', help='Filter objects to output by folder ID.')
@option(
    '-c',
    '--cluster',
    '--clusters',
    'cluster_ids',
    type=ListParamType(),
    help='Filter objects to output by one or several cluster IDs. Multiple values can be specified through a comma.',
)
@option(
    '--xcluster',
    '--exclude-cluster',
    '--exclude-clusters',
    'exclude_cluster_ids',
    type=ListParamType(),
    help='Filter objects to not output by one or several cluster IDs. Multiple values can be specified through a comma.',
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
@option(
    '--sc',
    '--subcluster',
    '--subclusters',
    'subcluster_ids',
    type=ListParamType(),
    help='Filter objects to output by one or several subcluster IDs. Multiple values can be specified through a comma.',
)
@option(
    '--xsubcluster',
    '--exclude-subcluster',
    '--exclude-subclusters',
    'exclude_subcluster_ids',
    type=ListParamType(),
    help='Filter objects to not output by one or several subcluster IDs. Multiple values can be specified through a comma.',
)
@option('-r', '--role', help='Filter objects to output by subcluster role.')
@option(
    '-H',
    '--host',
    '--hosts',
    'hostnames',
    type=ListParamType(),
    help='Filter objects to output by one or several host names. Multiple values can be specified through a comma.',
)
@option('--flavor-type', type=FlavorType(), help='Filter objects to output by flavor type.')
@option('--exclude-flavor-type', type=FlavorType(), help='Filter objects to not output by flavor type.')
@option(
    '--pillar',
    '--pillar-match',
    'pillar_filter',
    metavar='JSONPATH_EXPRESSION',
    help='Filter objects by matching pillar against the specified jsonpath expression. The matching is performed'
    ' using jsonb_path_match PostgreSQL function.',
)
@option('--min-host-count', type=int)
@option('--max-host-count', type=int)
@option('-l', '--limit', type=int, default=1000, help='Limit the max number of objects in the output.')
@option(
    '--fields',
    type=FieldsParamType(),
    default='id,roles,cluster_id,cluster_name,created_at,hosts',
    help='Fields to output. The value "all" equals to all available fields.'
    ' Default is "id,roles,cluster_id,cluster_name,created_at,hosts".',
)
@option('-q', '--quiet', is_flag=True, help='Output only subcluster IDs.')
@option('-s', '--separator', help='Value separator for quiet mode.')
@pass_context
def list_subclusters_command(
    ctx,
    untyped_ids,
    cluster_ids,
    subcluster_ids,
    hostnames,
    cluster_statuses,
    exclude_cluster_statuses,
    limit,
    fields,
    quiet,
    separator,
    **kwargs
):
    """List subclusters.

    For getting subclusters by related objects, IDs argument accepts cluster IDs, shard IDs and hostnames
    in addition to subcluster IDs."""
    if exclude_cluster_statuses is None and not any(
        (cluster_statuses, untyped_ids, cluster_ids, subcluster_ids, hostnames)
    ):
        exclude_cluster_statuses = DELETED_CLUSTER_STATUSES

    subclusters = get_subclusters(
        ctx,
        untyped_ids=untyped_ids,
        cluster_ids=cluster_ids,
        subcluster_ids=subcluster_ids,
        hostnames=hostnames,
        cluster_statuses=cluster_statuses,
        exclude_cluster_statuses=exclude_cluster_statuses,
        with_stats=True,
        limit=limit,
        **kwargs
    )
    print_response(
        ctx,
        subclusters,
        default_format='table',
        fields=fields,
        field_formatters=FIELD_FORMATTERS,
        ignored_fields=['pillar'],
        quiet=quiet,
        separator=separator,
    )
