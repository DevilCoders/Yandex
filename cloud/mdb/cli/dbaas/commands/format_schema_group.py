from click import argument, group, option, pass_context
from cloud.mdb.cli.common.formatting import print_response
from cloud.mdb.cli.dbaas.internal.intapi import perform_operation_rest
from cloud.mdb.cli.dbaas.internal.intapi import rest_request
from cloud.mdb.cli.dbaas.internal.metadb.cluster import get_cluster_type_by_id


@group('format-schema')
def format_schema_group():
    """Commands to manage ClickHouse format schemas."""
    pass


@format_schema_group.command('list')
@argument('cluster_id', metavar='CLUSTER')
@pass_context
def list_format_schemas_command(ctx, cluster_id):
    """List ClickHouse format schemas."""
    response = rest_request(ctx, 'GET', get_cluster_type_by_id(ctx, cluster_id), f'clusters/{cluster_id}/formatSchemas')
    print_response(ctx, response)


@format_schema_group.command('create')
@argument('cluster_id', metavar='CLUSTER')
@argument('format_schema')
@argument('type')
@argument('uri')
@option('--hidden', is_flag=True, help='Mark underlying task as hidden.')
@pass_context
def create_format_schema_command(ctx, cluster_id, format_schema, type, uri, hidden):
    """Create ClickHouse format schema."""
    cluster_type = get_cluster_type_by_id(ctx, cluster_id)
    perform_operation_rest(
        ctx,
        'POST',
        cluster_type,
        f'clusters/{cluster_id}/formatSchemas',
        hidden=hidden,
        data={
            'formatSchemaName': format_schema,
            'type': 'FORMAT_SCHEMA_TYPE_' + type,
            'uri': uri,
        },
    )


@format_schema_group.command('delete')
@argument('cluster_id', metavar='CLUSTER')
@argument('format_schema')
@option('--hidden', is_flag=True, help='Mark underlying task as hidden.')
@pass_context
def delete_format_schema_command(ctx, cluster_id, format_schema, hidden):
    """Delete ClickHouse format schema."""
    cluster_type = get_cluster_type_by_id(ctx, cluster_id)
    perform_operation_rest(
        ctx,
        'DELETE',
        cluster_type,
        f'clusters/{cluster_id}/formatSchemas/{format_schema}',
        hidden=hidden,
    )
