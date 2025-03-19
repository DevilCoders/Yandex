from click import argument, group, pass_context
from cloud.mdb.cli.common.formatting import print_response
from cloud.mdb.cli.dbaas.internal.intapi import perform_operation_rest
from cloud.mdb.cli.dbaas.internal.intapi import rest_request
from cloud.mdb.cli.dbaas.internal.metadb.cluster import get_cluster_type_by_id


@group('database')
def database_group():
    """Database management commands."""
    pass


@database_group.command('list')
@argument('cluster_id', metavar='CLUSTER')
@pass_context
def list_databases_command(ctx, cluster_id):
    """List databases."""
    response = rest_request(ctx, 'GET', get_cluster_type_by_id(ctx, cluster_id), f'clusters/{cluster_id}/databases')
    print_response(ctx, response)


@database_group.command('create')
@argument('cluster_id', metavar='CLUSTER')
@argument('database')
@pass_context
def create_database_command(ctx, cluster_id, database):
    """Create database."""
    cluster_type = get_cluster_type_by_id(ctx, cluster_id)
    perform_operation_rest(
        ctx,
        'POST',
        cluster_type,
        f'clusters/{cluster_id}/databases',
        data={
            'databaseSpec': {
                'name': database,
            },
        },
    )


@database_group.command('delete')
@argument('cluster_id', metavar='CLUSTER')
@argument('database')
@pass_context
def delete_database_command(ctx, cluster_id, database):
    """Delete database."""
    cluster_type = get_cluster_type_by_id(ctx, cluster_id)
    perform_operation_rest(ctx, 'DELETE', cluster_type, f'clusters/{cluster_id}/database/{database}')
