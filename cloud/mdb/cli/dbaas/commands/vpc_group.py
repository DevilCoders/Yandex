from click import argument, option
from click import group, pass_context

from cloud.mdb.cli.dbaas.internal.vpc import get_network, list_networks, get_operation, list_operations, wait_operation
from cloud.mdb.cli.common.formatting import print_response
from cloud.mdb.cli.common.parameters import FieldsParamType, ListParamType


@group('vpc')
def vpc_group():
    """VPC management commands."""
    pass


@vpc_group.group('network')
def network_group():
    """Networks management commands."""


@vpc_group.group('operation')
def operation_group():
    """Network operations management commands."""


@network_group.command('get')
@argument('network_id')
@pass_context
def get_network_command(ctx, network_id):
    """Get network."""
    print_response(ctx, get_network(ctx, network_id))


@network_group.command('list')
@option(
    '--project',
    '--projects',
    'project_ids',
    type=ListParamType(),
    help='Filter objects to output by one or several projects. Multiple values can be specified through a comma.',
)
@option(
    '-r',
    '--region',
    '--regions',
    'region_ids',
    type=ListParamType(),
    help='Filter objects to output by one or several regions. Multiple values can be specified through a comma.',
)
@option(
    '--status',
    '--statuses',
    'statuses',
    type=ListParamType(),
    help='Filter objects to output by one or several statuses. Multiple values can be specified through a comma.',
)
@option(
    '--fields',
    type=FieldsParamType(),
    default='network_id,name,project_id,cloud_provider,region_id,status,ipv4_cidr_block,ipv6_cidr_block',
    help='Fields to output. The value "all" equals to all available fields.'
    ' Default is "network_id,name,project_id,cloud_provider,region_id,status,ipv4_cidr_block,ipv6_cidr_block".',
)
@option('-l', '--limit', type=int, default=1000, help='Limit the max number of objects in the output.')
@option('-q', '--quiet', is_flag=True, help='Output only network IDs.')
@option('-s', '--separator', help='Value separator for quiet mode.')
@pass_context
def list_networks_command(ctx, project_ids, region_ids, statuses, fields, limit, quiet, separator):
    """List networks."""
    networks = list_networks(ctx, project_ids, region_ids, statuses, limit)
    print_response(
        ctx,
        networks,
        default_format='table',
        ignored_fields=[],
        fields=fields,
        quiet=quiet,
        separator=separator,
    )


@operation_group.command('get')
@argument('operation_id')
@pass_context
def get_operation_command(ctx, operation_id):
    """Get network."""
    print_response(ctx, get_operation(ctx, operation_id))


@operation_group.command('wait')
@argument('operation_id')
@pass_context
def wait_operation_command(ctx, operation_id):
    """Wait for operation completion."""
    op = wait_operation(ctx, operation_id)
    print_response(ctx, op)


@operation_group.command('list')
@option(
    '-n',
    '--network-id',
    '--network-ids',
    'network_ids',
    type=ListParamType(),
    help='Filter objects to output by one or several network_ids. Multiple values can be specified through a comma.',
)
@option(
    '--project',
    '--projects',
    'project_ids',
    type=ListParamType(),
    help='Filter objects to output by one or several projects. Multiple values can be specified through a comma.',
)
@option(
    '-r',
    '--region',
    '--regions',
    'region_ids',
    type=ListParamType(),
    help='Filter objects to output by one or several regions. Multiple values can be specified through a comma.',
)
@option(
    '--status',
    '--statuses',
    'statuses',
    type=ListParamType(),
    help='Filter objects to output by one or several statuses. Multiple values can be specified through a comma.',
)
@option(
    '--fields',
    type=FieldsParamType(),
    default='operation_id,action,metadata,create_time,cloud_provider,region,status',
    help='Fields to output. The value "all" equals to all available fields.'
    ' Default is "operation_id,action,metadata,create_time,cloud_provider,region,status".',
)
@option('-l', '--limit', type=int, default=1000, help='Limit the max number of objects in the output.')
@option('-q', '--quiet', is_flag=True, help='Output only network IDs.')
@option('-s', '--separator', help='Value separator for quiet mode.')
@pass_context
def list_operations_command(ctx, network_ids, project_ids, region_ids, statuses, fields, limit, quiet, separator):
    """List networks."""
    networks = list_operations(ctx, network_ids, project_ids, region_ids, statuses, limit)
    print_response(
        ctx,
        networks,
        default_format='table',
        ignored_fields=[],
        fields=fields,
        quiet=quiet,
        separator=separator,
    )
