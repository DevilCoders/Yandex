from click import argument, group, option, pass_context
from cloud.mdb.cli.common.formatting import print_response
from cloud.mdb.cli.dbaas.internal.config import config_option
from cloud.mdb.cli.dbaas.internal.intapi import ClusterType, rest_request
from cloud.mdb.cli.dbaas.internal.metadb.backup import create_backup_task


@group('backup')
def backup_group():
    """Backup management commands."""
    pass


@backup_group.command(name='list')
@argument('cluster_type', type=ClusterType())
@option('-f', '--folder', 'folder_id')
@pass_context
def list_backups_command(ctx, cluster_type, folder_id):
    """List backups."""
    if not folder_id:
        folder_id = config_option(ctx, 'compute', 'folder')

    response = rest_request(
        ctx,
        'GET',
        cluster_type,
        'backups',
        params={
            'folderId': folder_id,
        },
    )

    print_response(ctx, response)


@backup_group.command(name='create')
@option('-c', '--cluster', 'cluster_id')
@option('--shard', 'shard_id', default=None)
@pass_context
def create_backups_command(ctx, cluster_id, shard_id):
    """Create managed backup for cluster."""
    backup_id = create_backup_task(ctx, cluster_id=cluster_id, shard_id=shard_id)

    print(f'Cluster {cluster_id}: scheduled backup task {backup_id}')
