from click import group, option, pass_context

from cloud.mdb.cli.dbaas.internal import solomon
from cloud.mdb.cli.common.formatting import print_response


@group('solomon')
def solomon_group():
    """Solomon management commands."""
    pass


@solomon_group.group('shard')
def shard_group():
    """Shard management commands."""
    pass


@shard_group.command('list')
@option('-l', '--limit', default=1000, help='Limit the max number of objects in the output.')
@option('-q', '--quiet', is_flag=True, help='Output only shard IDs.')
@option('-s', '--separator', help='Value separator for quiet mode.')
@pass_context
def list_shards_command(ctx, limit, quiet, separator):
    """List Solomon shards."""
    print_response(ctx, solomon.get_shards(ctx, limit=limit), quiet=quiet, separator=separator)


@solomon_group.group('cluster')
def cluster_group():
    """Cluster management commands."""
    pass


@cluster_group.command('list')
@option('-l', '--limit', default=1000, help='Limit the max number of objects in the output.')
@option('-q', '--quiet', is_flag=True, help='Output only cluster IDs.')
@option('-s', '--separator', help='Value separator for quiet mode.')
@pass_context
def list_clusters_command(ctx, limit, quiet, separator):
    """List Solomon clusters."""
    print_response(
        ctx,
        solomon.get_clusters(ctx, limit=limit),
        default_format='table',
        fields=('id', 'name'),
        quiet=quiet,
        separator=separator,
    )
