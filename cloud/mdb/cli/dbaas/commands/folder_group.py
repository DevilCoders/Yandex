from click import argument, group, option, pass_context

from cloud.mdb.cli.common.formatting import print_response
from cloud.mdb.cli.common.parameters import ListParamType
from cloud.mdb.cli.dbaas.internal.metadb.folder import get_folder, get_folders


@group('folder')
def folder_group():
    """Folder management commands."""
    pass


@folder_group.command('get')
@argument('untyped_id', metavar='ID')
@pass_context
def get_folder_command(ctx, untyped_id):
    """Get folder.

    For getting folder by related object, ID argument accepts cluster ID in addition to folder ID.
    """
    print_response(ctx, get_folder(ctx, untyped_id=untyped_id))


@folder_group.command('list')
@option('--cloud', 'cloud_id', help='Filter folders to output by cloud.')
@option('-f', '--folder', '--folders', 'folder_ids', type=ListParamType(), help='Specify folders to output.')
@option(
    '-c',
    '--cluster',
    '--clusters',
    'cluster_ids',
    type=ListParamType(),
    help='Filter folders to output by one or several clusters. Multiple values can be specified through a comma.',
)
@option('-l', '--limit', default=1000, help='Limit the max number of objects in the output.')
@option('-q', '--quiet', is_flag=True, help='Output only folder IDs.')
@option('-s', '--separator', help='Value separator for quiet mode.')
@pass_context
def list_folders_command(ctx, limit, quiet, separator, **kwargs):
    """List folders."""
    folders = get_folders(ctx, limit=limit, **kwargs)
    print_response(ctx, folders, default_format='table', fields=['id', 'cloud_id'], quiet=quiet, separator=separator)
