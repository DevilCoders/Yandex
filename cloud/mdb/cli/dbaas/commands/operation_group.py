from click import argument, group, pass_context
from cloud.mdb.cli.dbaas.internal.intapi import rest_request
from cloud.mdb.cli.common.formatting import print_response
from cloud.mdb.cli.dbaas.internal.metadb.cluster import get_cluster_type_by_id
from cloud.mdb.cli.dbaas.internal.metadb.task import get_task


@group('operation')
def operation_group():
    """Operation management commands."""
    pass


@operation_group.command('get')
@argument('operation')
@pass_context
def get_operation_command(ctx, operation):
    """Get operation."""
    task = get_task(ctx, operation)
    cluster_type = get_cluster_type_by_id(ctx, task['cluster_id'])
    response = rest_request(ctx, 'GET', cluster_type, f'operations/{operation}')
    print_response(ctx, response)


@operation_group.command('list')
@argument('cluster_id', metavar='CLUSTER')
@pass_context
def list_operations_command(ctx, cluster_id):
    """List operations."""
    response = rest_request(ctx, 'GET', get_cluster_type_by_id(ctx, cluster_id), f'clusters/{cluster_id}/operations')
    print_response(ctx, response)
